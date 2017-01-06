# JSON REST service for Monero

Example of using [restbed](https://github.com/Corvusoft/restbed/) to provide Monero related JSON REST service.
For the example, a service called Open Monero was developed.

##  Open Monero 

Open Monero is an open source implementation of backend for
https://mymonero.com/. The frontend, html, css, JavaScript, were adapted
from, and originally developed by https://mymonero.com/. 

However, unlike MyMonero's backend, Open Monero's backend is open sourced, free
to use, host and modify. 

## Screenshot

![Open Monero]()

## Scrap notes

### Generate your own ssl certificate 
 
Setting up https and ssl certificates in restbed
 - https://github.com/Corvusoft/restbed/blob/34187502642144ab9f749ab40f5cdbd8cb17a54a/example/https_service/source/example.cpp

Based on the link above:
 
```bash
# Create Certificate
cd /tmp
openssl genrsa -out server.key 1024
openssl req -new -key server.key -out server.csr
openssl x509 -req -days 3650 -in server.csr -signkey server.key -out server.crt
openssl dhparam -out dh2048.pem 2048
```

### Enable it in Firefox or Chrome

Firefox and chrome will not work with that certificate as they cant verify it. 
To overcome this for development purposes on localhost, just open new tab in the browser used
and go to any link from the service, e.g., `https://localhost:1984/login`. Once you do this,
you should get warring about unsecured or un verified certificate. Then you just add it manually
as exception.

Also Open Monero generates uses 25 word seeds, which are fully comptabilite with `monero-wallet-cli`
and `monero-core`.
 
### Test connection using curl

Example of curl https request to the service

```bash
curl -k -X POST -d '{"withCredentials":true,"address":"41pJD13rU5r3KZsxzS65tL9zLMpZZCer8aWSi7wj8Xm99BAgXthcj2wgazxdTX9auFAmp3czfJUGH2S3UJfLwDWXUxc3ooC","view_key":"06d1f0f0fd766c75b52b9c597592d06f4bca5cd6dcd3e9bf1859bc78d0d5f80e","create_account":true}' https://localhost:1984/login
```

### Compile mysql c++ connector 
```
git clone https://github.com/mysql/mysql-connector-cpp.git
cd mysql-connector-cpp && mkdir build && cd build 
cmake .. -DBUILD_STATIC=1
make
```


## Other examples

Other examples can be found on  [github](https://github.com/moneroexamples?tab=repositories).
Please know that some of the examples/repositories are not
finished and may not work as intended.

## How can you help?

Constructive criticism, code and website edits are always good. They can be made through github.

Some Monero are also welcome:
```
48daf1rG3hE1Txapcsxh6WXNe9MLNKtu7W7tKTivtSoVLHErYzvdcpea2nSTgGkz66RFP4GKVAsTV14v6G3oddBTHfxP6tU
```
