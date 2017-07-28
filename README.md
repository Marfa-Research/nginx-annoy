nginx-annoy
=========

Nginx module for knn search using annoy.

### Build
```
git clone git@github.com:jquadrin/nginx-annoy.git /tmp/nginx-annoy
cd /tmp/nginx-annoy
make
```


### Build nginx
```
wget -qO - http://nginx.org/download/nginx-1.11.5.tar.gz | tar -xzf - -C /tmp
cd /tmp/nginx-1.11.5

./configure --with-compat --conf-path=/tmp/nginx-annoy/example.conf \
            --add-dynamic-module=/tmp/nginx-annoy \
            --with-ld-opt="-L/tmp/nginx-annoy/build -lannoy -lstdc++ -lm" \
            --with-cc-opt="-I/tmp/nginx-annoy/src"
            
make && make install 
```


### Configuration

#### Load module
```
load_module "modules/ngx_annoy_mod.so";
...
```
---
#### Load id map and annoy file
The id map is loaded from a file containing one id per line, each
id is assigned an integer from 0..n+1
```
http {
    annoy_init "/tmp/nginx-annoy/example_ids.txt" "/tmp/nginx-annoy/annoy/test/test.tree";
}
```
---
#### Search
`annoy_nns_by_uuid content_id n` Responds with a json object containing k recommendations for specified content id. Generates a json response like so `{"recs": ["content_id_1", ...]}`.
```
        location /nearest {
            annoy_nns_by_uuid $http_x_content_uuid $http_x_n; 
        }
```



