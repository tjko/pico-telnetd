/* SHA256-based Unix crypt implementation.
   Released into the Public Domain by Ulrich Drepper <drepper@redhat.com>.  */

#ifndef SHA256_CRYPT_H
#define SHA256_CRYPT_H


/* This entry point is equivalent to the `crypt' function in Unix
   libcs.  */
char *sha256_crypt(const char *key, const char *salt);


#endif /* SHA256_CRYPT_H */
