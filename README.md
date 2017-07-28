nginx-annoy
=========

Nginx module for knn search using annoy.

```
git clone git@github.com:marfa-research/nginx-annoy.git /tmp/nginx-annoy
cd /tmp/nginx-annoy

make


wget -qO - http://nginx.org/download/nginx-1.11.5.tar.gz | tar -xzf - -C /tmp
cd /tmp/nginx-1.11.5

./configure --with-compat --conf-path=/tmp/nginx-annoy/example.conf \
            --add-dynamic-module=/tmp/nginx-annoy \
            --with-ld-opt="-L/tmp/nginx-annoy/build -lannoy -lstdc++ -lm" \
            --with-cc-opt="-I/tmp/nginx-annoy/src"
            
make && make install 

```

Below is the example nginx configuration:

```
load_module "modules/ngx_annoy_mod.so";

worker_processes  1;

events {
    worker_connections  1024;
}

http {
    annoy_init "/srv/uuids.txt" "/srv/annoy/test/test.tree";

    server {
        listen       80;
        server_name  localhost;

        location /nearest {
            annoy_nns_by_uuid $http_x_content_uuid $http_x_n;
        }
    }
}
```
