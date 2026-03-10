# 🌸 SSPM REST API (v4.0.0-Sakura)

SSPM provides a standard REST API for integration with GUI frontends, CI/CD pipelines, and third-party tools.

## Base URL
The default endpoint is `http://localhost:7373`. You can change the port via the `SSPM_DAEMON_PORT` environment variable.

## Authentication
By default, the API is only accessible from `localhost`. For remote access, a bearer token is required:
```bash
Authorization: Bearer <your-amber-token>
```

## Endpoints

### 🩺 System Health
- **URL**: `/health`
- **Method**: `GET`
- **Response**: `200 OK`
```json
{
  "status": "ok",
  "version": "4.0.0-Sakura",
  "platform": "linux",
  "uptime": 3600
}
```

### 📦 Package Management

#### List Installed Packages
- **URL**: `/packages`
- **Method**: `GET`
- **Response**: `200 OK`
```json
[
  {
    "name": "ripgrep",
    "version": "13.0.0",
    "backend": "cargo",
    "description": "A line-oriented search tool"
  }
]
```

#### Search Packages
- **URL**: `/packages/search`
- **Method**: `GET`
- **Params**: `q=<query>`
- **Response**: `200 OK`

#### Install Package
- **URL**: `/packages/install`
- **Method**: `POST`
- **Body**: `{ "name": "nginx", "backend": "apt" }`
- **Response**: `202 Accepted`

### 🗄️ Repository Management

#### List Repositories
- **URL**: `/repos`
- **Method**: `GET`

#### Add Repository
- **URL**: `/repos`
- **Method**: `POST`
- **Body**: `{ "name": "my-repo", "url": "https://example.com/repo" }`

## Integration SDK
For quick integration, use the provided header-only C++ SDK or the Python `sspm-api` package.

```python
import sspm
client = sspm.Client(token="...")
client.install("ripgrep")
```
