Tired of your shells not having history?

Tired of having multiple netcat listeners on your attack box?

Ever wanted to share your shells with your script kiddie friends?

Then you should try Conchologist!

![](https://github.com/mrmoss/conchologist/raw/master/screenshot.png)

Take a look at src/main.cpp, there's two ports (command line args are on the way...):
- Reverse Handler Port (default 8080)
- Web Handler Port (default 8081)

The reverse handler port listens for shells while the web handler port listens for web clients.

Send all shells to the reverse handler address and then connect to the web handler address via a web browser to interact with the shell sessions.

Supports basic plaintext shells (standard shell fork with a socket and stream dups) as well as encrypted shells (specification coming soon).

Note on plaintext shells - This handler waits for an RSA public to be sent for encrypted shells. Plaintext shells will need to do one of two things:
- Send something immediately over the socket that is not an RSA public key (such as any byte that's not a '-').
- Wait for a 10 second timeout (see top of rev_handler.cpp to change this timeout).

How do encrypted sessions work?
- Handler waits for shell to send an RSA public key, should look like one of these:
   * "-----BEGIN RSA PUBLIC KEY-----\n...-----END RSA PUBLIC KEY-----\n"
   * "-----BEGIN PUBLIC KEY-----\n...-----END PUBLIC KEY-----\n"
- Handler generates an AES 256 session key, encrypts it with the RSA public key, and sends it to the shell.
- Handler generates an AES 256 session initial vector, encrypts it with the RSA public key, and sends it to the shell.
- All further communications between the handler and shell are with AES 256 (specification coming soon).

Until the encrypted session specification comes out:
Currently, everything is sent as the ascii hex string of the encrypted bytes followed by a newline...this isn't the best convention and will be changed soon. At least one example shell will be provided with the specification...
