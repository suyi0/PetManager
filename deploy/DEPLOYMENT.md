# PetManager Deployment Guide

## 1. Recommended topology

- Frontend static files: `Nginx`
- Backend API/WebSocket service: `Crow` on port `8081`
- Database: `MySQL`
- Process management: `systemd`

## 2. Suggested server paths

- Project root: `/opt/petmanager`
- Frontend build output: `/var/www/petmanager/frontend-dist`
- Nginx site config: `/etc/nginx/sites-available/petmanager.conf`
- systemd service file: `/etc/systemd/system/petmanager-backend.service`

## 3. Server preparation

Install the required packages on Ubuntu:

```bash
sudo apt update
sudo apt install -y nginx mysql-server nodejs npm build-essential cmake pkg-config
```

Install the backend dependencies required by Crow/MySQL/OpenSSL/Boost according to your environment.

## 4. Upload the project

Copy the repository to:

```bash
/opt/petmanager
```

Then place the runtime environment file at:

```bash
/opt/petmanager/.env
```

Important:

- Replace all development secrets before going live.
- Do not expose `.env` in the web root.
- Confirm the database host, port, username, password, and schema name are correct.

## 5. Build and start the backend

```bash
cd /opt/petmanager
chmod +x bin/build.sh bin/start.sh
./bin/build.sh
./bin/start.sh
```

If the backend starts successfully, it should listen on port `8081`.

## 6. Build the frontend

```bash
cd /opt/petmanager/pethospital/frontend
npm install
npm run build
```

After the build finishes, copy the generated files to the Nginx web root:

```bash
sudo mkdir -p /var/www/petmanager/frontend-dist
sudo cp -r dist/* /var/www/petmanager/frontend-dist/
```

## 7. Enable the systemd backend service

Copy the service file:

```bash
sudo cp deploy/systemd/petmanager-backend.service /etc/systemd/system/
```

Reload and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable petmanager-backend
sudo systemctl start petmanager-backend
sudo systemctl status petmanager-backend
```

Check logs:

```bash
sudo journalctl -u petmanager-backend -f
```

## 8. Configure Nginx

Copy the Nginx config:

```bash
sudo cp deploy/nginx/petmanager.conf /etc/nginx/sites-available/petmanager.conf
sudo ln -s /etc/nginx/sites-available/petmanager.conf /etc/nginx/sites-enabled/petmanager.conf
```

Then test and reload:

```bash
sudo nginx -t
sudo systemctl reload nginx
```

Before using it, replace:

- `your-domain.com`
- `/var/www/petmanager/frontend-dist`

## 9. Enable HTTPS with Let's Encrypt

Prepare the certificate directory:

```bash
sudo mkdir -p /var/www/certbot
```

Install Certbot:

```bash
sudo apt install -y certbot python3-certbot-nginx
```

Copy the HTTPS Nginx template:

```bash
sudo cp deploy/nginx/petmanager-ssl.conf /etc/nginx/sites-available/petmanager.conf
```

Before reloading Nginx, replace:

- `your-domain.com`
- `/var/www/petmanager/frontend-dist`
- `/etc/letsencrypt/live/your-domain.com/fullchain.pem`
- `/etc/letsencrypt/live/your-domain.com/privkey.pem`

If this is the first certificate request, it is safer to request the certificate first using the domain name:

```bash
sudo certbot certonly --webroot -w /var/www/certbot -d your-domain.com -d www.your-domain.com
```

Then verify and reload Nginx:

```bash
sudo nginx -t
sudo systemctl reload nginx
```

To enable automatic renewal:

```bash
sudo systemctl enable certbot.timer
sudo systemctl start certbot.timer
systemctl list-timers | grep certbot
```

You can also test renewal manually:

```bash
sudo certbot renew --dry-run
```

## 10. Domain binding checklist

- Point the domain A record to the server public IP.
- Open ports `80` and `443` in the cloud firewall/security group.
- Ensure Nginx is reachable from the public network.
- Confirm the backend service is running locally on `127.0.0.1:8081`.
- Do not expose port `8081` to the public network unless needed.

## 11. Recommended production notes

- Use HTTPS with Let's Encrypt after domain binding.
- Keep frontend and backend under the same domain to reduce cross-origin issues.
- Open ports `80` and `443` in the cloud firewall.
- Do not expose port `8081` directly to the public network unless necessary.
- Back up MySQL regularly.

## 12. Thesis-ready deployment description

The system adopts a front-end and back-end separated deployment architecture. The frontend is packaged into static files and hosted by Nginx, while the backend service developed with C++ and Crow runs as an independent process managed by systemd. MySQL is used for persistent data storage, and Nginx forwards `/api`, `/uploads`, and `/websocket` requests to the backend service, enabling unified browser access under a single domain name.

For production deployment, the system further introduces HTTPS secure transmission based on domain name binding and Let's Encrypt certificates. Nginx is responsible for SSL termination and HTTP-to-HTTPS redirection, which improves communication security and ensures that browser access, user authentication, and business data transmission are protected in a real network environment.
