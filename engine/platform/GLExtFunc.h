//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


#ifndef GL_GROUP_BEGIN
#define GL_GROUP_BEGIN( flag )
#define UNDEF_BEGIN
#endif

#ifndef GL_GROUP_END
#define GL_GROUP_END()
#define UNDEF_END
#endif

//ARB_multitexture
GL_GROUP_BEGIN(ARB_multitexture)
GL_FUNCTION(void,       glActiveTextureARB, (GLenum target), return; )
GL_FUNCTION(void,       glClientActiveTextureARB, (GLenum target), return; )
//GL_FUNCTION(void,       glMultiTexCoord1dARB, (GLenum target, GLdouble s), return; )
//GL_FUNCTION(void,       glMultiTexCoord1dvARB, (GLenum target, const GLdouble *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord1fARB, (GLenum target, GLfloat s), return; )
//GL_FUNCTION(void,       glMultiTexCoord1fvARB, (GLenum target, const GLfloat *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord1iARB, (GLenum target, GLint s), return; )
//GL_FUNCTION(void,       glMultiTexCoord1ivARB, (GLenum target, const GLint *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord1sARB, (GLenum target, GLshort s), return; )
//GL_FUNCTION(void,       glMultiTexCoord1svARB, (GLenum target, const GLshort *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord2dARB, (GLenum target, GLdouble s, GLdouble t), return; )
//GL_FUNCTION(void,       glMultiTexCoord2dvARB, (GLenum target, const GLdouble *v), return; )
GL_FUNCTION(void,       glMultiTexCoord2fARB, (GLenum target, GLfloat s, GLfloat t), return; )
GL_FUNCTION(void,       glMultiTexCoord2fvARB, (GLenum target, const GLfloat *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord2iARB, (GLenum target, GLint s, GLint t), return; )
//GL_FUNCTION(void,       glMultiTexCoord2ivARB, (GLenum target, const GLint *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord2sARB, (GLenum target, GLshort s, GLshort t), return; )
//GL_FUNCTION(void,       glMultiTexCoord2svARB, (GLenum target, const GLshort *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord3dARB, (GLenum target, GLdouble s, GLdouble t, GLdouble r), return; )
//GL_FUNCTION(void,       glMultiTexCoord3dvARB, (GLenum target, const GLdouble *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord3fARB, (GLenum target, GLfloat s, GLfloat t, GLfloat r), return; )
//GL_FUNCTION(void,       glMultiTexCoord3fvARB, (GLenum target, const GLfloat *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord3iARB, (GLenum target, GLint s, GLint t, GLint r), return; )
//GL_FUNCTION(void,       glMultiTexCoord3ivARB, (GLenum target, const GLint *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord3sARB, (GLenum target, GLshort s, GLshort t, GLshort r), return; )
//GL_FUNCTION(void,       glMultiTexCoord3svARB, (GLenum target, const GLshort *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord4dARB, (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q), return; )
//GL_FUNCTION(void,       glMultiTexCoord4dvARB, (GLenum target, const GLdouble *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord4fARB, (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q), return; )
//GL_FUNCTION(void,       glMultiTexCoord4fvARB, (GLenum target, const GLfloat *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord4iARB, (GLenum target, GLint s, GLint t, GLint r, GLint q), return; )
//GL_FUNCTION(void,       glMultiTexCoord4ivARB, (GLenum target, const GLint *v), return; )
//GL_FUNCTION(void,       glMultiTexCoord4sARB, (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q), return; )
//GL_FUNCTION(void,       glMultiTexCoord4svARB, (GLenum target, const GLshort *v), return; )
GL_GROUP_END()

//ARB_texture_compression
GL_GROUP_BEGIN(ARB_texture_compression)
GL_FUNCTION(void,			glCompressedTexImage3DARB, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void*), return; )
GL_FUNCTION(void,			glCompressedTexImage2DARB, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void*), return; )
GL_FUNCTION(void,			glCompressedTexImage1DARB, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLsizei imageSize, const void*), return; )
GL_FUNCTION(void,			glCompressedTexSubImage3DARB, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void*), return; )
GL_FUNCTION(void,			glCompressedTexSubImage2DARB, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void*), return; )
GL_FUNCTION(void,			glCompressedTexSubImage1DARB, (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void*), return; )
GL_FUNCTION(void,			glGetCompressedTexImageARB, (GLenum target, GLint lod, void* img), return; )
GL_GROUP_END()

//EXT_compiled_vertex_array
GL_GROUP_BEGIN(EXT_compiled_vertex_array)
GL_FUNCTION(void,       glLockArraysEXT, (GLint first, GLsizei count), return; )
GL_FUNCTION(void,       glUnlockArraysEXT, (void), return; )
GL_GROUP_END()

//EXT_fog_coord
GL_GROUP_BEGIN(EXT_fog_coord)
GL_FUNCTION(void,			glFogCoordfEXT, (GLfloat), return; )
GL_FUNCTION(void ,		glFogCoordPointerEXT, (GLenum, GLsizei, void*), return; )
GL_GROUP_END()

//EXT_paletted_texture
GL_GROUP_BEGIN(EXT_paletted_texture)
GL_FUNCTION(void,       glColorTableEXT, (GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void* data), return; )
GL_GROUP_END()

//NV_vertex_array_range
#ifdef TORQUE_OS_WIN32
GL_GROUP_BEGIN(NV_vertex_array_range)
GL_FUNCTION(void, glVertexArrayRangeNV, (GLsizei length, void* pointer), return; )
GL_FUNCTION(void, glFlushVertexArrayRangeNV, (), return; )
GL_FUNCTION(void*, wglAllocateMemoryNV, (GLsizei, GLfloat, GLfloat, GLfloat), return NULL; )
GL_FUNCTION(void, wglFreeMemoryNV, (void*), return; )
GL_GROUP_END()
#endif

#ifdef UNDEF_BEGIN
#undef GL_GROUP_BEGIN
#undef UNDEF_BEGIN
#endif

#ifdef UNDEF_END
#undef GL_GROUP_END
#undef UNDEF_END
#endif
