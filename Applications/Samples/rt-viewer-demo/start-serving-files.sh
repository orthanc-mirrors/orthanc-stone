#!/bin/bash

sudo nginx -p $(pwd) -c nginx.local.conf

echo "Please browse to :"

echo "http://localhost:9977/rt-viewer-demo.html?ct-series=a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa&dose-instance=830a69ff-8e4b5ee3-b7f966c8-bccc20fb-d322dceb&struct-instance=54460695-ba3885ee-ddf61ac0-f028e31d-a6e474d9"

echo "(This requires you have uploaded the correct files to your local Orthanc instance)"
