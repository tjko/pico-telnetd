# pico-telnetd
Experimental Telnet server for Raspberry Pi Pico W. This library was created for [FanPico](https://github.com/tjko/fanpico/) project.

## Features

*pico_telnetd* provides a light-weight Telnet (or "Raw TCP") server for Pico W.  This is meant to be used with the Pico-SDK.

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
tcp_server_t *telnetserver = telnet_server_init(2048, 8192); // input and output buffer sizes
if (!telnetserver)
    panic("out of memory);

telnetserver->mode = TELNET_MODE;
telnet_server_start(telnetserver, true);  // parameter tells whether to enable stdio driver or not... */

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


cp_server_t *telnetserver = telnet_server_init(2048, 8192);
if (!telnetserver)
    panic("out of memory);

telnetserver->mode = TELNET_MODE;
telnetserver->port = 8000;
telnetserver->auth_cb = sha512crypt_auth_cb;
telnetserver->auth_cb_param = (void*)users;

telnet_server_start(telnetserver, true);  // parameter tells whether to enable stdio driver or not... */

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
tcp_server_start(telnetserver, true);
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
telnet_server_start(telnetserver, true);
...
```


#### Disabling logging completely

To disable logging done by the library completely. Simply set the _log_cb_ to NULL.

```
...
telnetserver->log_cb = NULL;

telnet_server_start(telnetserver, true);
...
```


### Usage withouth SDTIO

Telnet server can alternatively be used withouth stdio, by setting stdio parameter to _false_:

```
telnet_server_start(telnetserver, false);
```

#### Reading Data Received from Client

Received data is store in the _rb_in_ ringbuffer. 

Ringbuffer can be read charcter by character using _telnet_ringbuffer_read_char()_ function:
```
...
int in;
while ((in = telnet_ringbuffer_read_char(&telnetserver->rb_in)) >= 0) {
  printf("Received byte: %02x (%c)\n", in, isprint(in) ? in : '?');
}
...
```

Alternatively larger blocks can be read from ring buffer using _telnet_ringbuffer_read()_ function:
```
size_t bytes_waiting = telnet_ringbuffer_size(&telnetserver->rb_in);
if (bytes_waiting > 0) {
  telnet_ringbuffer_read(&telnetserver->rb_in, buffer, bytes_waiting);  // make sure buffer is large enough...
}
```


#### Sending Data to Client

Data added to ringbuffer _rb_out_, will be transmitted to the client.

Data can be added to ringbuffer either one character at the time using _telnet_ringbuffer_add_char()_ function:
```
for (int i = 0; i < strlen(buf); i++) {
	telnet_ringbuffer_add_char(&telnetserver->rb_out, buf[i], true);  // Last argument controls wheter to overwrite in case ringbuffer fills uup...
}
```

Or larger blocks can be sent uainf _telnet_ringbuffer_add()_ function:
```
// add data to ringbuffer withouth overwriting data if buffer woud fill up (overwrite parameter set to false)
int err = telnet_ringbuffer_add(&telnetserver->rb_out, buf, buffer_len, false);
if (err != 0) {
   // buffer would fill up 
}
```

## Examples
See [src/telnetd.c](https://github.com/tjko/fanpico/blob/main/src/telnetd.c) in FanPico project for actual usage example.

