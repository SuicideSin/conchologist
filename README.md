Tired of your shells not having history?

Tired of having multiple netcat listeners on your attack box?

Ever wanted to share your shells with your script kiddie friends?

Then you should try Conchologist!

![](https://github.com/mrmoss/conchologist/raw/master/screenshot.png)

Take a look at src/main.cpp, has two ports:
- Reverse Handler Port (default 8080)
- Web Handler Port (default 8081)

The reverse handler port listens for shells while the web handler port listens for web clients.
Send all shells to the reverse handler address and then connect to the web handler address to interact with the shell sessions!