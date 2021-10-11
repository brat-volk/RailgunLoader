# RailgunLoader
C++ Multi-Stage Malware Loader/Dropper.
----------------------------------------------------
The loader is designed to work just like a Railgun, with the projectile being invisibly manipulated by electromagnetic coils.
The First Stage Loader drops 3 files: a vbs script, a batch script and a zip file.
After a reboot the bat file gets autorun at startup and the vbs unzips the zipfile, then the batch script deletes all traces of the loader.
Then the unzipped exe from the zipfile is run by the batch file and it finally delivers the last compressed exe(the payload) and starts it indirectly using a batch file and the registry.
