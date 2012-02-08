#include "AlembicArchiveStorage.h"
#include <map>
#include <boost/algorithm/string.hpp>
#include "AlembicLicensing.h"

struct AlembicArchiveInfo
{
   Alembic::Abc::IArchive * archive;
   int refCount;

   AlembicArchiveInfo()
   {
      archive = NULL;
      refCount = 0;
   }
};
std::map<std::string,AlembicArchiveInfo> gArchives;

MString resolvePath(MString path)
{
   // TODO: does maya support tokens?
   return path;
}

Alembic::Abc::IArchive * getArchiveFromID(MString path)
{
   MString resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath.asChar());
   if(it == gArchives.end())
   {
      // check if we can open more archives
      if(!HasFullLicense())
      {
         if(gArchives.size() == 2)
         {
            EC_LOG_WARNING("[ExocortexAlembic] Demo Mode: Only two open archives at a time allowed!");
            return NULL;
         }
      }

      // check if the file exists
      FILE * file = fopen(resolvedPath.asChar(),"rb");
      if(file != NULL)
      {
         fclose(file);
         addArchive(new Alembic::Abc::IArchive( Alembic::AbcCoreHDF5::ReadArchive(), resolvedPath.asChar()));
         return getArchiveFromID(resolvedPath);
      }
      return NULL;
   }
   return it->second.archive;
}

MString addArchive(Alembic::Abc::IArchive * archive)
{
   AlembicArchiveInfo info;
   info.archive = archive;
   gArchives.insert(std::pair<std::string,AlembicArchiveInfo>(archive->getName(),info));
   return archive->getName().c_str();
}

void deleteArchive(MString path)
{
   MString resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath.asChar());
   if(it == gArchives.end())
      return;
   it->second.archive->reset();
   delete(it->second.archive);
   gArchives.erase(it);
}

void deleteAllArchives()
{
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   for(it = gArchives.begin(); it != gArchives.end(); it++)
   {
      it->second.archive->reset();
      delete(it->second.archive);
   }
   gArchives.clear();
}

Alembic::Abc::IObject getObjectFromArchive(MString path, MString identifier)
{
   Alembic::Abc::IArchive * archive = getArchiveFromID(path);
   if(archive == NULL)
      return Alembic::Abc::IObject();

   // split the path
   std::string stdIdentifier(identifier.asChar());
   std::vector<std::string> parts;
   boost::split(parts, stdIdentifier, boost::is_any_of("/"));

   // recurse to find it
   Alembic::Abc::IObject obj = archive->getTop();
   for(size_t i=1;i<parts.size();i++)
   {
      Alembic::Abc::IObject child(obj,parts[i]);
      obj = child;
      if(!obj)
         break;
   }

   return obj;
}

int addRefArchive(MString path)
{
   if(path.length() == 0)
      return -1;
   MString resolvedPath = resolvePath(path);

   // call get archive to ensure to create it!
   getArchiveFromID(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath.asChar());
   if(it == gArchives.end())
      return -1;
   it->second.refCount++;
   return it->second.refCount;
}

int delRefArchive(MString path)
{
   MString resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath.asChar());
   if(it == gArchives.end())
      return -1;
   it->second.refCount--;
   if(it->second.refCount==0)
   {
      deleteArchive(resolvedPath);
      return 0;
   }
   return it->second.refCount;
}

int getRefArchive(MString path)
{
   MString resolvedPath = resolvePath(path);
   std::map<std::string,AlembicArchiveInfo>::iterator it;
   it = gArchives.find(resolvedPath.asChar());
   if(it == gArchives.end())
      return -1;
   return it->second.refCount;
}