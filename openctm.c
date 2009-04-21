//-----------------------------------------------------------------------------
// Product:     OpenCTM
// File:        openctm.c
// Description: API functions.
//-----------------------------------------------------------------------------
// Copyright (c) 2009 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//     1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//
//     2. Altered source versions must be plainly marked as such, and must not
//     be misrepresented as being the original software.
//
//     3. This notice may not be removed or altered from any source
//     distribution.
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "openctm.h"
#include "internal.h"

//-----------------------------------------------------------------------------
// _ctmFreeMapList() - Free a float map list.
//-----------------------------------------------------------------------------
static void _ctmFreeMapList(_CTMcontext * self, _CTMfloatmap * aMapList)
{
  _CTMfloatmap * map, * nextMap;
  map = aMapList;
  while(map)
  {
    // Free internally allocated array (if we are in import mode)
    if((self->mMode == CTM_IMPORT) && map->mValues)
      free(map->mValues);

    // Free map name
    if(map->mName)
      free(map->mName);

    nextMap = map->mNext;
    free(map);
  }
}

//-----------------------------------------------------------------------------
// _ctmClearMesh() - Clear the mesh in a CTM context.
//-----------------------------------------------------------------------------
static void _ctmClearMesh(_CTMcontext * self)
{
  // Free internally allocated mesh arrays
  if(self->mMode == CTM_IMPORT)
  {
    if(self->mVertices)
      free(self->mVertices);
    if(self->mIndices)
      free(self->mIndices);
    if(self->mNormals)
      free(self->mNormals);
  }

  // Clear externally assigned mesh arrays
  self->mVertices = (CTMfloat *) 0;
  self->mVertexCount = 0;
  self->mIndices = (CTMuint *) 0;
  self->mTriangleCount = 0;
  self->mNormals = (CTMfloat *) 0;

  // Free texture coordinate map list
  _ctmFreeMapList(self, self->mTexMaps);
  self->mTexMaps = (_CTMfloatmap *) 0;
  self->mTexMapCount = 0;

  // Free attribute map list
  _ctmFreeMapList(self, self->mAttribMaps);
  self->mAttribMaps = (_CTMfloatmap *) 0;
  self->mAttribMapCount = 0;
}

//-----------------------------------------------------------------------------
// ctmNewContext()
//-----------------------------------------------------------------------------
CTMcontext ctmNewContext(CTMcontextmode aMode)
{
  _CTMcontext * self;

  // Allocate memory for the new structure
  self = (_CTMcontext *) malloc(sizeof(_CTMcontext));

  // Initialize structure (set null pointers and zero array lengths)
  memset(self, 0, sizeof(_CTMcontext));
  self->mMode = aMode;
  self->mError = CTM_NO_ERROR;
  self->mMethod = CTM_METHOD_MG1;
  self->mVertexPrecision = 1.0 / 1024.0;

  return (CTMcontext) self;
}

//-----------------------------------------------------------------------------
// ctmFreeContext()
//-----------------------------------------------------------------------------
void ctmFreeContext(CTMcontext aContext)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

  // Free all mesh resources
  _ctmClearMesh(self);

  // Free the file comment
  if(self->mFileComment)
    free(self->mFileComment);

  // Free the context
  free(self);
}

//-----------------------------------------------------------------------------
// ctmError()
//-----------------------------------------------------------------------------
CTMerror ctmError(CTMcontext aContext)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  CTMerror err;

  if(!self) return CTM_INVALID_CONTEXT;

  // Get error code and reset error state
  err = self->mError;
  self->mError = CTM_NO_ERROR;
  return err;
}

//-----------------------------------------------------------------------------
// ctmGetInteger()
//-----------------------------------------------------------------------------
CTMuint ctmGetInteger(CTMcontext aContext, CTMproperty aProperty)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return 0;

  switch(aProperty)
  {
    case CTM_VERTEX_COUNT:
      return self->mVertexCount;

    case CTM_TRIANGLE_COUNT:
      return self->mTriangleCount;

    case CTM_TEX_MAP_COUNT:
      return self->mTexMapCount;

    case CTM_ATTRIB_MAP_COUNT:
      return self->mAttribMapCount;

    case CTM_HAS_NORMALS:
      return self->mNormals ? CTM_TRUE : CTM_FALSE;

    default:
      self->mError = CTM_INVALID_ARGUMENT;
  }

  return 0;
}

//-----------------------------------------------------------------------------
// ctmGetIntegerArray()
//-----------------------------------------------------------------------------
const CTMuint * ctmGetIntegerArray(CTMcontext aContext, CTMproperty aProperty)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return (CTMuint *) 0;

  switch(aProperty)
  {
    case CTM_INDICES:
      return self->mIndices;

    default:
      self->mError = CTM_INVALID_ARGUMENT;
  }

  return (CTMuint *) 0;
}

//-----------------------------------------------------------------------------
// ctmGetFloatArray()
//-----------------------------------------------------------------------------
const CTMfloat * ctmGetFloatArray(CTMcontext aContext, CTMproperty aProperty)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  _CTMfloatmap * map;
  CTMuint i;
  if(!self) return (CTMfloat *) 0;

  // Did the user request a texture map?
  if((aProperty >= CTM_TEX_MAP_1) &&
     ((aProperty - CTM_TEX_MAP_1) < self->mTexMapCount))
  {
    map = self->mTexMaps;
    i = CTM_TEX_MAP_1;
    while(map && (i != aProperty))
    {
      map = map->mNext;
      ++ i;
    }
    if(!map)
    {
      self->mError = CTM_INVALID_ARGUMENT;
      return (CTMfloat *) 0;
    }
    return map->mValues;
  }

  // Did the user request an attribute map?
  if((aProperty >= CTM_ATTRIB_MAP_1) &&
     ((aProperty - CTM_ATTRIB_MAP_1) < self->mAttribMapCount))
  {
    map = self->mAttribMaps;
    i = CTM_ATTRIB_MAP_1;
    while(map && (i != aProperty))
    {
      map = map->mNext;
      ++ i;
    }
    if(!map)
    {
      self->mError = CTM_INVALID_ARGUMENT;
      return (CTMfloat *) 0;
    }
    return map->mValues;
  }

  switch(aProperty)
  {
    case CTM_VERTICES:
      return self->mVertices;

    case CTM_NORMALS:
      return self->mNormals;

    default:
      self->mError = CTM_INVALID_ARGUMENT;
  }

  return (CTMfloat *) 0;
}

//-----------------------------------------------------------------------------
// ctmGetString()
//-----------------------------------------------------------------------------
const char * ctmGetString(CTMcontext aContext, CTMproperty aProperty)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return 0;

  switch(aProperty)
  {
    case CTM_FILE_COMMENT:
      return (const char *) self->mFileComment;

    default:
      self->mError = CTM_INVALID_ARGUMENT;
  }

  return (const char *) 0;
}

//-----------------------------------------------------------------------------
// ctmCompressionMethod()
//-----------------------------------------------------------------------------
void ctmCompressionMethod(CTMcontext aContext, CTMmethod aMethod)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

  // You are only allowed to change compression attributes in export mode
  if(self->mMode != CTM_EXPORT)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check arguments
  if((aMethod != CTM_METHOD_RAW) && (aMethod != CTM_METHOD_MG1) &&
     (aMethod != CTM_METHOD_MG2))
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Set method
  self->mMethod = aMethod;
}

//-----------------------------------------------------------------------------
// ctmVertexPrecision()
//-----------------------------------------------------------------------------
void ctmVertexPrecision(CTMcontext aContext, CTMfloat aPrecision)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

  // You are only allowed to change compression attributes in export mode
  if(self->mMode != CTM_EXPORT)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check arguments
  if(aPrecision <= 0.0)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Set precision
  self->mVertexPrecision = aPrecision;
}

//-----------------------------------------------------------------------------
// ctmVertexPrecisionRel()
//-----------------------------------------------------------------------------
void ctmVertexPrecisionRel(CTMcontext aContext, CTMfloat aRelPrecision)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  CTMfloat avgEdgeLength, * p1, * p2;
  CTMuint edgeCount, i, j;
  if(!self) return;

  // You are only allowed to change compression attributes in export mode
  if(self->mMode != CTM_EXPORT)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check arguments
  if(aRelPrecision <= 0.0)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Calculate the average edge length (Note: we actually sum up all the half-
  // edges, so in a proper solid mesh all connected edges are counted twice)
  avgEdgeLength = 0.0;
  edgeCount = 0;
  for(i = 0; i < self->mTriangleCount; ++ i)
  {
    p1 = &self->mVertices[self->mIndices[i * 3 + 2] * 3];
    for(j = 0; j < 3; ++ j)
    {
      p2 = &self->mVertices[self->mIndices[i * 3 + j] * 3];
      avgEdgeLength += sqrt((p2[0] - p1[0]) * (p2[0] - p1[0]) +
                            (p2[1] - p1[1]) * (p2[1] - p1[1]) +
                            (p2[2] - p1[2]) * (p2[2] - p1[2]));
      p1 = p2;
      ++ edgeCount;
    }
  }
  if(edgeCount == 0)
  {
    self->mError = CTM_INVALID_MESH;
    return;
  }
  avgEdgeLength /= (CTMfloat) edgeCount;

  // Set precision
  self->mVertexPrecision = aRelPrecision * avgEdgeLength;
}

//-----------------------------------------------------------------------------
// ctmTexCoordPrecision()
//-----------------------------------------------------------------------------
void ctmTexCoordPrecision(CTMcontext aContext, CTMproperty aTexMap,
  CTMfloat aPrecision)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

  // You are only allowed to change compression attributes in export mode
  if(self->mMode != CTM_EXPORT)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check arguments
  if((aPrecision <= 0.0) || (aTexMap < CTM_TEX_MAP_1) ||
     ((aTexMap - CTM_TEX_MAP_1) >= self->mTexMapCount))
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // FIXME!
}

//-----------------------------------------------------------------------------
// ctmAttribPrecision()
//-----------------------------------------------------------------------------
void ctmAttribPrecision(CTMcontext aContext, CTMproperty aAttribMap,
  CTMfloat aPrecision)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

  // You are only allowed to change compression attributes in export mode
  if(self->mMode != CTM_EXPORT)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check arguments
  if((aPrecision <= 0.0) || (aAttribMap < CTM_ATTRIB_MAP_1) ||
     ((aAttribMap - CTM_ATTRIB_MAP_1) >= self->mAttribMapCount))
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // FIXME!
}

//-----------------------------------------------------------------------------
// ctmFileComment()
//-----------------------------------------------------------------------------
void ctmFileComment(CTMcontext aContext, const char * aFileComment)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  int len;
  if(!self) return;

  // You are only allowed to change file attributes in export mode
  if(self->mMode != CTM_EXPORT)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Free the old comment string, if necessary
  if(self->mFileComment)
  {
    free(self->mFileComment);
    self->mFileComment = (char *) 0;
  }

  // Get length of string (if empty, do nothing)
  if(!aFileComment)
    return;
  len = strlen(aFileComment);
  if(!len)
    return;

  // Copy the string
  self->mFileComment = (char *) malloc(len + 1);
  if(!self->mFileComment)
  {
    self->mError = CTM_OUT_OF_MEMORY;
    return;
  }
  strcpy(self->mFileComment, aFileComment);
}

//-----------------------------------------------------------------------------
// ctmDefineMesh()
//-----------------------------------------------------------------------------
void ctmDefineMesh(CTMcontext aContext, const CTMfloat * aVertices,
                   CTMuint aVertexCount, const CTMuint * aIndices,
                   CTMuint aTriangleCount, const CTMfloat * aNormals)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

  // You are only allowed to (re)define the mesh in export mode
  if(self->mMode != CTM_EXPORT)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check arguments
  if(!aVertices || !aIndices || !aVertexCount || !aTriangleCount)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Clear the old mesh, if any
  _ctmClearMesh(self);

  // Set vertex array pointer
  self->mVertices = (CTMfloat *) aVertices;
  self->mVertexCount = aVertexCount;

  // Set index array pointer
  self->mIndices = (CTMuint *) aIndices;
  self->mTriangleCount = aTriangleCount;

  // Set normal array pointer
  self->mNormals = (CTMfloat *) aNormals;
}

//-----------------------------------------------------------------------------
// ctmAddTexMap()
//-----------------------------------------------------------------------------
CTMproperty ctmAddTexMap(CTMcontext aContext, const CTMfloat * aTexCoords,
  const char * aName)
{
  // FIXME!
  return CTM_NONE;
}

//-----------------------------------------------------------------------------
// ctmAddAttribMap()
//-----------------------------------------------------------------------------
CTMproperty ctmAddAttribMap(CTMcontext aContext, const CTMfloat * aAttribValues,
  const char * aName)
{
  // FIXME!
  return CTM_NONE;
}

//-----------------------------------------------------------------------------
// _ctmDefaultRead()
//-----------------------------------------------------------------------------
static CTMuint _ctmDefaultRead(void * aBuf, CTMuint aCount, void * aUserData)
{
  return (CTMuint) fread(aBuf, 1, (size_t) aCount, (FILE *) aUserData);
}

//-----------------------------------------------------------------------------
// ctmLoad()
//-----------------------------------------------------------------------------
void ctmLoad(CTMcontext aContext, const char * aFileName)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  FILE * f;
  if(!self) return;

  // You are only allowed to load data in import mode
  if(self->mMode != CTM_IMPORT)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Open file stream
  f = fopen(aFileName, "rb");
  if(!f)
  {
    self->mError = CTM_FILE_ERROR;
    return;
  }

  // Load the file
  ctmLoadCustom(self, _ctmDefaultRead, (void *) f);

  // Close file stream
  fclose(f);
}

//-----------------------------------------------------------------------------
// ctmLoadCustom()
//-----------------------------------------------------------------------------
void ctmLoadCustom(CTMcontext aContext, CTMreadfn aReadFn, void * aUserData)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  CTMuint formatVersion, flags, method;
  if(!self) return;

  // You are only allowed to load data in import mode
  if(self->mMode != CTM_IMPORT)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Initialize stream
  self->mReadFn = aReadFn;
  self->mUserData = aUserData;

  // Clear any old mesh arrays
  _ctmClearMesh(self);

  // Read header from stream
  if(_ctmStreamReadUINT(self) != FOURCC("OCTM"))
  {
    self->mError = CTM_FORMAT_ERROR;
    return;
  }
  formatVersion = _ctmStreamReadUINT(self);
  if(formatVersion != _CTM_FORMAT_VERSION)
  {
    self->mError = CTM_FORMAT_ERROR;
    return;
  }
  method = _ctmStreamReadUINT(self);
  if(method == FOURCC("RAW\0"))
    self->mMethod = CTM_METHOD_RAW;
  else if(method == FOURCC("MG1\0"))
    self->mMethod = CTM_METHOD_MG1;
  else if(method == FOURCC("MG2\0"))
    self->mMethod = CTM_METHOD_MG2;
  else
  {
    self->mError = CTM_FORMAT_ERROR;
    return;
  }
  self->mVertexCount = _ctmStreamReadUINT(self);
  if(self->mVertexCount == 0)
  {
    self->mError = CTM_FORMAT_ERROR;
    return;
  }
  self->mTriangleCount = _ctmStreamReadUINT(self);
  if(self->mTriangleCount == 0)
  {
    self->mError = CTM_FORMAT_ERROR;
    return;
  }
  self->mTexMapCount = _ctmStreamReadUINT(self);
  self->mAttribMapCount = _ctmStreamReadUINT(self);
  flags = _ctmStreamReadUINT(self);
  _ctmStreamReadSTRING(self, &self->mFileComment);

  // Allocate memory for the mesh arrays
  self->mVertices = (CTMfloat *) malloc(self->mVertexCount * sizeof(CTMfloat) * 3);
  if(!self->mVertices)
  {
    self->mError = CTM_OUT_OF_MEMORY;
    return;
  }
  self->mIndices = (CTMuint *) malloc(self->mTriangleCount * sizeof(CTMuint) * 3);
  if(!self->mIndices)
  {
    _ctmClearMesh(self);
    self->mError = CTM_OUT_OF_MEMORY;
    return;
  }
  if(flags & _CTM_HAS_NORMALS_BIT)
  {
    self->mNormals = (CTMfloat *) malloc(self->mVertexCount * sizeof(CTMfloat) * 3);
    if(!self->mNormals)
    {
      _ctmClearMesh(self);
      self->mError = CTM_OUT_OF_MEMORY;
      return;
    }
  }

  // Allocate memory for texture maps
  // FIXME!

  // Allocate memory for attribute maps
  // FIXME!

  // Uncompress from stream
  switch(self->mMethod)
  {
    case CTM_METHOD_RAW:
      _ctmUncompressMesh_RAW(self);
      break;

    case CTM_METHOD_MG1:
      _ctmUncompressMesh_MG1(self);
      break;

    case CTM_METHOD_MG2:
      _ctmUncompressMesh_MG2(self);
      break;
  }
}

//-----------------------------------------------------------------------------
// _ctmDefaultWrite()
//-----------------------------------------------------------------------------
static CTMuint _ctmDefaultWrite(const void * aBuf, CTMuint aCount,
  void * aUserData)
{
  return (CTMuint) fwrite(aBuf, 1, (size_t) aCount, (FILE *) aUserData);
}

//-----------------------------------------------------------------------------
// ctmSave()
//-----------------------------------------------------------------------------
void ctmSave(CTMcontext aContext, const char * aFileName)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  FILE * f;
  if(!self) return;

  // You are only allowed to save data in export mode
  if(self->mMode != CTM_EXPORT)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Open file stream
  f = fopen(aFileName, "wb");
  if(!f)
  {
    self->mError = CTM_FILE_ERROR;
    return;
  }

  // Save the file
  ctmSaveCustom(self, _ctmDefaultWrite, (void *) f);

  // Close file stream
  fclose(f);
}

//-----------------------------------------------------------------------------
// ctmSaveCustom()
//-----------------------------------------------------------------------------
void ctmSaveCustom(CTMcontext aContext, CTMwritefn aWriteFn, void * aUserData)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  CTMuint flags;
  if(!self) return;

  // You are only allowed to save data in export mode
  if(self->mMode != CTM_EXPORT)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check mesh integrity
  if(!self->mVertices || !self->mIndices || (self->mVertexCount < 1) ||
     (self->mTriangleCount < 1))
  {
    self->mError = CTM_INVALID_MESH;
    return;
  }

  // Initialize stream
  self->mWriteFn = aWriteFn;
  self->mUserData = aUserData;

  // Determine flags
  flags = 0;
  if(self->mNormals)
    flags |= _CTM_HAS_NORMALS_BIT;

  // Write header to stream
  _ctmStreamWrite(self, (void *) "OCTM", 4);
  _ctmStreamWriteUINT(self, _CTM_FORMAT_VERSION);
  switch(self->mMethod)
  {
    case CTM_METHOD_RAW:
      _ctmStreamWrite(self, (void *) "RAW\0", 4);
      break;
    case CTM_METHOD_MG1:
      _ctmStreamWrite(self, (void *) "MG1\0", 4);
      break;
    case CTM_METHOD_MG2:
      _ctmStreamWrite(self, (void *) "MG2\0", 4);
      break;
  }
  _ctmStreamWriteUINT(self, self->mVertexCount);
  _ctmStreamWriteUINT(self, self->mTriangleCount);
  _ctmStreamWriteUINT(self, self->mTexMapCount);
  _ctmStreamWriteUINT(self, self->mAttribMapCount);
  _ctmStreamWriteUINT(self, flags);
  _ctmStreamWriteSTRING(self, self->mFileComment);

  // Compress to stream
  switch(self->mMethod)
  {
    case CTM_METHOD_RAW:
      _ctmCompressMesh_RAW(self);
      break;

    case CTM_METHOD_MG1:
      _ctmCompressMesh_MG1(self);
      break;

    case CTM_METHOD_MG2:
      _ctmCompressMesh_MG2(self);
      break;
  }
}
