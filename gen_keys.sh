#!/bin/bash
openssl genrsa -out private_key.pem 4096
openssl rsa -pubout -in private_key.pem -out public_key.pem
chmod 600 private_key.pem public_key.pem