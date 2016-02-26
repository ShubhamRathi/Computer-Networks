#!/bin/bash
chmod 777 *
echo "Script will now run make clean and make"
make clean
make
echo "Compiled Successfully!"