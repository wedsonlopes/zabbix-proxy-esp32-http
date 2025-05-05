# zabbix-proxy-esp32-http
Sending esp32 data to zabbix via HTTP

Requirements pre-installed:

- Zabbix Server 
- Zabbix Proxy

* **How to configure nginx in zabbix proxy?**
```
yum install zabbix-sender -y
yum install nginx php-fpm -y
yum install -y php php-fpm php-cli php-json php-common php-opcache php-mbstring -y
```
```
mkdir -p /usr/local/www
```

```
vim /usr/local/www/zabbix_trap.php
```

```
<?php
// Accept only POST
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['error' => 'Only POST allowed']);
    exit;
}

// Read JSON input
$input = file_get_contents('php://input');
$data = json_decode($input, true);

// Validate input
if (!isset($data['host'], $data['key'], $data['value'])) {
    http_response_code(400);
    echo json_encode(['error' => 'Missing host, key, or value']);
    exit;
}

// Escape inputs
$host = escapeshellarg($data['host']);
$key = escapeshellarg($data['key']);
$value = escapeshellarg($data['value']);

// Use full path to zabbix_sender
$zabbixSender = '/usr/bin/zabbix_sender';  // change if needed
$zabbixServer = 'zbx.company.com';     // your Zabbix server URL or IP Address

// Prepare command
$cmd = "$zabbixSender -z $zabbixServer -s $host -k $key -o $value";

// Log the incoming command for debugging
file_put_contents('/tmp/zabbix_trap_debug.log', "CMD: $cmd\n", FILE_APPEND);

// Run command and capture output
exec($cmd . " 2>&1", $output, $exitCode);

// Log detailed output
$log = date('c') . "\nCMD: $cmd\nExit Code: $exitCode\n" . implode("\n", $output) . "\n\n";
file_put_contents('/tmp/zabbix_trap_debug.log', $log, FILE_APPEND);

// Output result
if ($exitCode === 0) {
    echo json_encode(['status' => 'OK', 'sent' => true]);
} else {
    http_response_code(500);
    echo json_encode(['status' => 'FAIL', 'output' => $output]);
}
?>
```

* **How to configure nginx to handle PHP?**

Comment default nginx config:

```
#    server {
#        listen       80 default_server;
#        listen       [::]:80 default_server;
#        server_name  _;
#        root         /usr/share/nginx/html;
#
#        # Load configuration files for the default server block.
#        include /etc/nginx/default.d/*.conf;
#
#        location / {
#        }
#
#        error_page 404 /404.html;
#            location = /40x.html {
#        }
#
#        error_page 500 502 503 504 /50x.html;
#            location = /50x.html {
#        }
#    }

```
Get your php-fpm path in: `/etc/php-fpm.d/www.conf`
```
listen = /run/php-fpm/www.sock
```
Edit your `vim /etc/php.ini` `disable_functions = `
```
disable_functions = passthru,shell_exec,system
```
Create a new nginx config file:
```
cd /etc/nginx/conf.d/
vim zabbix-trapper.conf
```
```
upstream php-handler {
    # The default PHP-FPM socket is often /run/php/php-fpm.sock, but if you are using www.sock, make sure PHP-FPM is listening on this.
    server unix:/run/php-fpm/www.sock; 
}

server {
    listen 8000 default_server;
    listen [::]:8000 default_server;  # IPv6 support (if needed)

    root /usr/local/www/;           # Document root
    index index.php index.html index.htm;  # Index files

    server_name _;                  # Default server (you can set a specific domain here)

    location / {
        try_files $uri $uri/ =404;  # Serve static files and show a 404 if not found
    }

    # PHP handling
    location ~ \.php$ {
        fastcgi_pass php-handler;  # This passes PHP requests to the upstream PHP-FPM handler
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;  # Ensure correct PHP file path
        include fastcgi_params;  # Include the standard FastCGI parameters for PHP-FPM
    }

    location ~ /\.ht {  # Deny access to .ht files
        deny all;
    }
}

```
* **Enable the services and restart to test it**
```
systemctl enable nginx php-fpm

systemctl restart nginx php-fpm

or
systemctl enable --now nginx php-fpm
```
* **Create your Host in zabbix server**

Host Name: ESP32-SENSOR

- Create an Item
```
Item name: Temperature
Item Type: Zabbix Trapper
Item Key: temperature
Type of Information: Numeric (Unsigned)
```

* **Now open yourt arduino APP and compile the .ino sketch yo your ESP32 board.**
