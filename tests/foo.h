
void gendep__register_open (char const *fn, int flags, int fd/*, int existed_if_rw*/);
void gendep__register_close (int fd);
void gendep__register_read (int fd, void * buf, size_t count);
void gendep__register_write (int fd, __const void * buf, size_t count);
void gendep__register_rename (const char * oldname, const char * newname);
