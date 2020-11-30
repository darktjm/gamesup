/*
 * An LD_PRELOAD hack for Imperium Glactica II:  Alliacnes.  This game
 * declars its shaders to be too high a version (4.6 in my case) for the
 * gl_FragColor variable to be legal.  I edit every such shader to make it
 * work.  For now, I add " compatibility" to the version number, which,
 * according to Mesa at least, enables this variable.
 * To build:
 *     gcc -O2 -fPIC -shared -o ig2-hack.{so,c} -ldl -lGLEW -lGL
 *
 * To use:
 *   LD_PRELOAD=/path/to/lg2-hack.so ./ig2
 */

/* for RTLD_NEXT and glibc's memmem */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* <sigh> can't get just this from GL.h due to glew */
#undef glShaderSource
/* GLAPI */ void /* APIENTRY */ glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);


/* texture2DLod/textureLod are 1.3 functions, so advertise users as such */
static /* GLAPI */ void /* APIENTRY */ hack_glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length)
{
    GLsizei n, len, vern, verlen, verend;
    const GLchar **mstring = NULL;
    GLchar *rstring = NULL, *ver = NULL, *t2 = NULL;

    for(n = 0; n < count; n++) {
	len = length && length[n] >= 0 ? length[n] : strlen(string[n]);
	if(!ver && (ver = memmem(string[n], len, "#version ", 9))) {
	    if(!isdigit(ver[9]) || !isdigit(ver[10]) || !isdigit(ver[11]) ||
	       ver[9] < '4') {
		t2 = NULL;
		break;
	    }
	    vern = n;
	    verlen = len;
	    verend = ver - string[n] + 12;
	}
	if((t2 = memmem(string[n], len, "gl_FragColor", 12)))
	    break;
    }
    if(t2 && ver) {
	const char compat[] = " compatibility";
	mstring = malloc(count * sizeof(*mstring));
	rstring = malloc(verlen + sizeof(compat));
	if(!mstring || !rstring) {
	    perror("shader replacement");
	    exit(1);
	}
	memcpy(mstring, string, count * sizeof(*string));
	mstring[vern] = rstring;
	memcpy(rstring, string[vern], verend);
	memcpy(rstring + verend, compat, sizeof(compat) - 1);
	memcpy(rstring + verend + sizeof(compat) - 1, string[vern] + verend, verlen - verend);
	rstring[verlen + sizeof(compat) - 1] = 0;
	string = mstring;
    }
    glShaderSource(shader, count, string, length);
    if(mstring) {
	free(mstring);
	free(rstring);
    }
}

GLenum GLEWAPIENTRY glewInit (void)
{
    GLenum ret = ((GLenum GLEWAPIENTRY (*)(void))dlsym(RTLD_NEXT, "glewInit"))();
    __glewShaderSource = hack_glShaderSource;
    fputs("ShaderSource intercepted\n", stderr);
    return ret;
}
