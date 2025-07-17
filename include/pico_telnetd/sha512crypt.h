/* SHA512-based Unix crypt implementation.
   Released into the Public Domain by Ulrich Drepper <drepper@redhat.com>.  */

#ifndef SHA512_CRYPT_H
#define SHA512_CRYPT_H

#ifdef __cplusplus
extern "C"
{
#endif


/* This entry point is equivalent to the `crypt' function in Unix
   libcs.  */
char *sha512_crypt(const char *key, const char *salt);


#ifdef __cplusplus
}
#endif

#endif /* SHA512_CRYPT_H */
