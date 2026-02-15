# HttpdWin
## Windows HTTP server with Python backend.     


Compile using visual studio.   
Supports only python >= 3.13.    



### Pre-requisites:    
1. Openssl 3       
2. Python 3 (Install it using the installer from python website)   
3. Visual Studio Express (For compilation, you should find the solution file under HttpdWin/HttpdWin.sln )     
     
     
### Execution Requirements:    
Unpack the Release zip file to C:\ drive and find some test pages and script that was used for validation.    
You need to create public key and private key files, use openssl commands to create them , it should be in PEM format.    



### NOTE:    
As of now it runs only from C:\ drive    
Does not use MinGW neither does it use MSys or WSL.    
If you are observing a crash, it is because certificate and key files are missing from C:\HttpdWin\Certs\     
There may be memory leaks.    



### Last Updated : 02/15/2026 or In Indian date format 15/02/2026      



## How to setup HttpdWin
#### Setting up the program 
     Download the release package from release link.
     Uncompress to C:\Httpdwin\
     Make sure all .exe, libs and .dll fils are in C:\HttpdWin\Bin\



#### Setting up the config and test scripts and html files
     C:\HttpdWin\Bin\    <- Location of the binaries and libraries.     
     C:\HttpdWin\Certs\  <- Where the certificates and key files go.    
     C:\HttpdWin\Pages\  <- Where all the script files (in python) and html files go , similar to /var/www/html for apache in linux.    
     C:\HttpdWin\Storage <- Where cookie data is stored (clear it , if you don't want old cookies to be loaded).   
     C:\HttpdWin\Temp\   <- Where all the http temp files are created, example post upload, json files, put files ... etc (this can be cleared often).    
     C:\HttpdWin\Tests\  <- Where loadtest.py a load testing script written in python is present, uses GNU wget for Win64.   
     C:\HttpdWin\httpdwin.conf  <- Simple configuration file for httpd server.  
     C:\HttpdWin\httpdwin-errors.log <- Log file of HttpdWin.



##### Configuration looks like this:
     #number of threads to launch in httpdwin program         
     threads=40     
     #http port number for non ssl connections         
     httpport=8080     
     #https port number for ssl connections      
     httpsport=8081    
     #ipv4 bind address         
     bindipv4=127.0.0.1    
     #ipv6 bind address     
     bindipv6=::1     
     #debug level : 1-5    
     debuglevel=1     
     #transport mechanism : secure / non-secure / both     
     transport=both         
     #session cookie name     
     session=SID     
     #max age     
     maxage=3600     



#### Creating Certificates and Key files:
     The following certificate and key files are required:
     httpdwinkey.pem
     httpdwinpub.pem     
     httpdwincert.pem     
     httpdwinkey6.pem      
     httpdwinpub6.pem      
     httpdwincert6.pem   
     
     openssl.exe req -x509 -newkey rsa:4096 -keyout httpdwinkey.pem -out httpdwincert.pem -sha256 -days 3650 -config openssl.cnf      
     openssl.exe x509 -pubkey -noout -in httpdwincert.pem  > httpdwinpub.pem      
     
     copy httpdwinkey.pem  C:\HttpdWin\Certs\httpdwinkey.pem    
     copy httpdwinpub.pem  C:\HttpdWin\Certs\httpdwinpub.pem     
     copy httpdwincert.pem C:\HttpdWin\Certs\httpdwincert.pem     
     copy httpdwinkey.pem  C:\HttpdWin\Certs\httpdwinkey6.pem      
     copy httpdwinpub.pem  C:\HttpdWin\Certs\httpdwinpub6.pem      
     copy httpdwincert.pem C:\HttpdWin\Certs\httpdwincert6.pem     



#### Run the executable
     Open command prompt     
     cd C:\HttpdWin\Bin\     
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

     Python global variables:    
     input             - The input file which contains all information regarding HTTP query request, including headers, cookie information and query information
     sessionid         - The variable that contains the value of the current session ID.
     

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


#### Log messages & it's interpretation     
     Operational logs are stored in C:\\HttpdWin\httpdwin-errors.log     
     Message level - 
     ERROR - Error message      
     WARN  - Warning message     
     "    " / INFO - Informational message    
     DEBUG - Debug message     
     XTRA  - Additional Debug message    

     Log message format:    
     <Level>   : <Time in microseconds since Epoch (local time) > : <Thread ID, (Optional, If printed from thread)> : <Meaningful Log message>      
     WARN  : 1771069602950973 : 37: Assigned thread completed job, rejoining threadpool      



