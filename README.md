# HTTP Server & Common Gateway Interface

It is a HTTP server designed to execute the specified CGI program.
The result can be view from a browser that support HTML5.
The server support all commands in [link](https://github.com/Timmy50301/NP_Linux_Shell).

##  Usage

To make the sample program:
```bash
make
```

Run the http_server on 140.113.194.210 and port 5000
```bash
bash$ ./http_server 5000
```

Assume your http_server is running on 140.113.194.210 and listening at port 5000.

Open a browser and visit, http://[HTTP_server_host]:[port]/[CGI_name].cgi 

The web page will automatically redirected to specified CGI program.
```bash
http://140.113.194.210:5000/hello.cgi
```
```bash
http://140.113.194.210:5000/printenv.cgi
```
```bash
http://140.113.194.210:5000/welcome.cgi
```

For "panel.cgi" and "console.cgi", CGIs will connect automatically to the remote servers with shell prompt "% ", and the files contain the commands for the remote shells, similar to [link](https://github.com/Timmy50301/NP_Linux_Shell). Make sure you execute the np_single_golden in the rwg(remote working groung).
```bash
bash$ ./np_single_golden 1234 # assume it is running on 140.113.194.220:1234
```
```bash
bash$ ./np_single_golden 5678 # assume it is running on 140.113.194.230:5678
```
```bash
http://140.113.194.210:5000/panel.cgi
```
console.cgi will parse the URL with corresponding connecting information to the remote working ground
```bash
http://140.113.194.210:5000/console.cgi?h0=140.113.194.220:1234&p0=1234&f0=t1.txt&h1=140.113.194.230:5678&p1=5678&f1=t2.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=
```

