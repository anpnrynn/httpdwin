# HttpdWin
## Windows HTTP server with Python backend.    

Reboot of my 2007 server, writing a cleaner code.   

Compile using visual studio.      

### Pre-requisites:    
1. Openssl   
2. Python 3
3. Visual Studio Express (For compilation, you should find the solution file under HttpdWin/HttpdWin.sln )
     
     
### Execution Requirements:    
Put in the need .dll files where the executable is and run it.    
The prerequisite binary packages has all that is needed.     



Unpack the HttpdWin.zip to C:\ drive and find some test pages and script that was used for validation.    
You need to create public key and private key files, use openssl commands to create them , it should be in PEM format.


As of now, Load tests haven't been done.    
There may be memory leaks. 

### NOTE: 
As of now it runs only from C:\ drive    
Does not use MinGW neither does it use MSys or WSL.    


### Last Updated : 02/12/2026 or In Indian format 12/02/2026      


## How to setup HttpdWin
#### Setting up the program 
     Download the release from release link.
     Uncompress to C:\Httpdwin-Bin\
     Make sure all .exe, libs and .dll fils are in C:\HttpdWin-Bin\

#### Setting up the config and test scripts and html files
     Download Httpdwin.zip from the repo, by download the raw file and uncompress it to C:\HttpdWin\
     C:\HttpdWin\Certs\  <- Where the certificates and key files go
     C:\HttpdWin\Pages\  <- Where all the script files (in python) and html files go , similar to /var/www/html for apache in linux
     C:\HttpdWin\Storage <- Where cookie data is stored (clear it , if you don't want old cookies to be loaded)
     C:\HttpdWin\Temp\   <- Where all the http temp files are created, example post upload, json files, put files ... etc (this can be cleared often)
     C:\HttpdWin\httpdwin.conf  <- Simple configuration file for httpd server

##### Configuration looks like this:
     #number of threads to launch in httpdwin program         
     threads=4     
     #http port number for non ssl connections         
     httpport=8080     
     #https port number for ssl connections      
     httpsport=8081    
     #ipv4 bind address         
     bindipv4=127.0.0.1    
     #ipv6 bind address     
     bindipv6=::1     
     #debug level : 1-5    
     debuglevel=4     
     #transport mechanism : secure / non-secure / both     
     transport=both      

#### Creating Certificates and Key files:
     openssl.exe req -x509 -newkey rsa:4096 -keyout httpdwinkey.pem -out httpdwincert.pem -sha256 -days 3650 -config openssl.cnf      
     openssl.exe x509 -pubkey -noout -in httpdwincert.pem  > httpdwinpub.pem      
     Copy httpdwinkey.pem  C:\HttpdWin\Certs\httpdwinkey.pem    
     Copy httpdwinpub.pem  C:\HttpdWin\Certs\httpdwinpub.pem     
     Copy httpdwincert.pem C:\HttpdWin\Certs\httpdwincert.pem     
     Copy httpdwinkey.pem  C:\HttpdWin\Certs\httpdwinkey6.pem      
     Copy httpdwinpub.pem  C:\HttpdWin\Certs\httpdwinpub6.pem      
     Copy httpdwincert.pem C:\HttpdWin\Certs\httpdwincert6.pem     

#### Run the executable
     Open command prompt     
     cd C:\HttpdWin-Bin\     
     .\HttpdWin.exe     

#### Python HttpdWin functions that are exposed to Python3 scripts
     wwwprint          - Prints (ascii) onto the www browser     
     wwwbytesprint     - Prints byte data (binary data) onto the www browser     
     wwwprintend       - Prints the final content onto the www browser     
     wwwheadercomplete - Sends the header to the web browser         
     wwwmime           - Sets Content-Type field     
     wwwheaderadd      - Sets headers, Omit the carriage return and linefeed     
     wwwcookieset      - Sets a cookie     
     wwwcookiedel      - Deletes a cookie    
     wwwsessionclear   - Clears all cookies of a session    

#### Sample Python script file from HttpdWin.zip in the repo
     import HttpdWin    
     HttpdWin.wwwmime ("text/html")    
     HttpdWin.wwwheaderadd ("Set-Value: Helloworld A")    
     HttpdWin.wwwheaderadd ("Set-Value-B: Helloworld B")    
     HttpdWin.wwwheadercomplete ()    
     HttpdWin.wwwprint("<html><head></head><body><h1>This code works from python !!!</h1><h2>")    
     HttpdWin.wwwprint(sessionid)    
     HttpdWin.wwwprint("<br/>"+input+"<br/>")     
     HttpdWin.wwwprint("</h2>")    
     import json    
     with open(input, 'r') as file:    
         data = json.load(file)    
     HttpdWin.wwwprint("<code>");    
     HttpdWin.wwwprint("JSON DATA:");    
     HttpdWin.wwwprint(str(data));     
     HttpdWin.wwwprint("</code>");     
     HttpdWin.wwwprint("</body></html>")    
     HttpdWin.wwwprintend("");     
     HttpdWin.wwwsessionclear();     



