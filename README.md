# JSON REST service for Monero stuff over https

Example of using [restbed](https://github.com/Corvusoft/restbed/) to provide Monero related JSON REST service.

# Scrap notes
Setting up https and ssl certificates in restbed
 - https://github.com/Corvusoft/restbed/blob/34187502642144ab9f749ab40f5cdbd8cb17a54a/example/https_service/source/example.cpp

From the link above:
 
```bash
# Create Certificate
cd /tmp
openssl genrsa -out server.key 1024
openssl req -new -key server.key -out server.csr
openssl x509 -req -days 3650 -in server.csr -signkey server.key -out server.crt
openssl dhparam -out dh768.pem 768
```
 
Example of curl https request

```bash
curl -k -X POST -H "Content-Type: application/json" -d '{"jsonrpc": "2.0", "id": "test", "method": "notifyServer","params": {}}' https://localhost:1984/resource
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
