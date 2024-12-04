
#include "LittleFSsupport.h"
#define DEBUG

static Stream* debugPtr = NULL;  // local to this file
static bool FS_initialized = false;

/* ===================
r   Open a file for reading. If a file is in reading mode, then no data is deleted if a file is already present on a system.
r+  open for reading and writing from beginning

w   Open a file for writing. If a file is in writing mode, then a new file is created if a file doesn’t exist at all. 
    If a file is already present on a system, then all the data inside the file is truncated, and it is opened for writing purposes.
w+  open for reading and writing, overwriting a file

a   Open a file in append mode. If a file is in append mode, then the file is opened. The content within the file doesn’t change.
a+  open for reading and writing, appending to file
====================== */

void logError(const String& errorMessage) {
    File logFile = LittleFS.open("/error.log", "a");
    if (logFile) {
        logFile.println(errorMessage);
        logFile.close();
    }
}
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
   Serial.println("LittleFS mounted successfully");

  }
}

void listDir(const char * dirname) {
  listDir(dirname, *debugPtr);
}

void listDir(const char * dirname, Stream& out) {
  if (!FS_initialized) {
    out.println("FS not initialized yet");
    return;
  }
  out.printf("Listing directory: %s\n", dirname);
  File root = LittleFS.open(dirname);
  if ( root.isDirectory() ) {
    String file1 = root.getNextFileName();
    while (file1 != "") {
      File file = LittleFS.open(file1,"r");
      out.print("  FILE: ");
      out.print(file.name());
      out.print("  SIZE: ");
      out.println(file.size());
      file.close();
      file1 = root.getNextFileName();
    }
  }
  out.println();
}
/// @brief : renames the file in little FS
/// @param path1 : original name of the file to be renamed
/// @param path2 : Character string of new name to be given
/// @return : false if fails in renaming otherwise true
bool renameFile(const char * path1, const char * path2) {
  if (!FS_initialized) {
    return false;
  }
  if (LittleFS.rename(path1, path2)) {
    return true;
  } //else
  return false;
}

bool deleteFile(const char * path) {
  if (!FS_initialized) {
    return false;
  }
  if (LittleFS.remove(path)) {
    return true;
  }
  //else
  return false;
}
