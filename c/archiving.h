/*****************************************************************************

 Public definitions that are common to all types of archives.

 *****************************************************************************/
#ifndef _ARCHIVING_H_
#define _ARCHIVING_H_

enum archive_insert_result_t {
    ARCHIVE_INSERT_UKNOWN_ERROR = -4,
    ARCHIVE_INSERT_X_MUST_BE_NOT_NULL = -3,
    ARCHIVE_INSERT_X_MUST_BE_NULL = -2,
    ARCHIVE_INSERT_MEMORY_ERROR = -1,
    ARCHIVE_INSERT_REJECTED = 0,
    ARCHIVE_INSERT_ACCEPTED = 1,
    ARCHIVE_INSERT_DUPLICATED = 2,
};

#endif // _ARCHIVING_H_
