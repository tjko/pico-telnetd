# pico-telnetd
Telnet Server for Raspberry Pi Pico W

## Features

*pico_telnetd* provides a light-weidht Telnet (or "Raw TCP") server for Pico W.  This is meant to be used with the Pico-SDK.

* Provides _stdio_ driver so a Telnet (or raw TCP) connection can be used as "console" for Pico W.
* "Interface" library that is easy to include in existing Pico-SDK projects.
* Support authentication via authentication callback. Includes support for (Linux) SHA-512 Crypt and SHA-256 Crypt password hashes.


## Usage

### Including _pico_telnetd_ in a project

First, get the library (this example adds it as a submodule to existing repository):
```
$ mkdir libs
$ cd libs
$ git submodule add https://github.com/tjko/pico-telnetd.git
```

Then to use _pico_telnetd_, add it in your CMakeLists.txt:
```
# include pico-telnetd library
add_subdirectory(libs/pico-telnetd)
```

### Telnet Server (no authentication) as STDIO device

To add "telnet" support to a Pico W project, that allows accessing the unit over a Telnet connection just like from Serial or USB (CDC) connection

```
#include <pico_telnetd.h>
...
<initialize networking>
...
tcp_server_t *telnetserver = telnet_server_init();
if (!telnetserver)
    panic("out of memory);

telnetserver->mode = TELNET_MODE;
telnet_server_start(true);  // parameter tells whether to enable stdio driver or not... */

```


### Telnet Server with authentication on a non-standard port

To enable authentication, we must specify a callback function that handles the authentication. This example uses the included example callback that uses list of
login/pwhash pairs to authenticate against.

```
#include <pico_telnetd.h>
#include <pico_telnetd/util.h>

/* list of users and their (Linux) SHA-512 Crypt password hashes... (this example used "admin/admin") */
user_pwhash_entry_t users[] = {
	{ "admin", "$6$caRtcnraEpbI48d3$YizNnV2hIwqZ/Gu4jh9ebV/DXCRhCzvUM2E0yTF3BgGrMw1HrfYIJJ9CQ0rcVBbpScCfwBtKhynVpKSnW/5o.." },
	{ NULL, NULL }
};


cp_server_t *telnetserver = telnet_server_init();
if (!telnetserver)
    panic("out of memory);

telnetserver->mode = TELNET_MODE;
telnetserver->port = 8000;
telnetserver->auth_cb = sha512crypt_auth_cb;
telnetserver->auth_cb_param = (void*)users;

telnet_server_start(true);  // parameter tells whether to enable stdio driver or not... */

```

## Additional Features

### Authentication
pico_telnetd support simple password based authentication. To enable authentication _auth_cb_ needs to be set 
(and optionally _auth_cb_param_ to specify parameters to pass to the authentication function.

```
int my_auth_cb(void *param, const char *login, const char *password)
{
  if <user password is valid> {
    return 0; 
  } 
  return -1;
}

...

tcpserver->auth_cb = my_auth_cb;
tcp_server_start(true);
...
```


### Logging
#### Controlling Logging

By default _pico-telnetd_ logs errors to stdout. Loggin verbosity can be configured by setting logging level:
```
#include <pico_telnetd/log.h>

telnetd_log_level(LOG_INFO);
```
(see pico_telnetd/log.h for the logging levels)


#### Custom logging

To completely control logging, it is possible to provide custom callback for the logging:
```
void my_logger(int priority, const char *format, ...)
{
<implement custom logging here>
}

...
telnetserver->log_cb = my_logger;
telnet_server_start(true);
...
```


#### Disabling logging completely

To disable logging done by the library completely. Simply set the _log_cb_ to NULL.

```
...
telnetserver->log_cb = NULL;

telnet_server_start(true);
...
```


```
