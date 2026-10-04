# Deploy JoeScript Studio

The frontend is a static Vercel site. The compiler API runs separately in the Linux container defined by the repository-root `Dockerfile`.

## Deploy the compiler API

Create a Docker web service from this repository on a container host such as Render. Use the repository root as the build context and `Dockerfile` as the Dockerfile path. The service listens on the platform-provided `PORT` and exposes `GET /health` for its health check.

Set `ALLOWED_ORIGIN` to the exact production Vercel origin, with no trailing slash, for example `https://joescript-studio.vercel.app`. The API only enables cross-origin requests from that origin. For Vercel preview deployments, update this value to the preview origin you are testing.

For a local image build and run:

```sh
docker build -t joescript-backend .
docker run --rm -p 4173:4173 -e ALLOWED_ORIGIN=http://localhost:3000 joescript-backend
```

The backend also serves the frontend as a local fallback at `http://localhost:4173`.

## Deploy the frontend

Create a Vercel project connected to this repository and set its **Root Directory** to `src/frontend`. The included `vercel.json` builds the static site. Add the environment variable `JOESCRIPT_API_URL` with the backend's public origin, for example `https://joescript-api.onrender.com`, then deploy or redeploy.

The frontend sends runs to `JOESCRIPT_API_URL/api/run`; keep the backend's `ALLOWED_ORIGIN` equal to the Vercel site's origin.

## Security note

This container is a deployment starting point, not a complete hostile-code sandbox. Submitted JoeScript is compiled to native code and executed by the API process's operating-system user. Do not expose the run endpoint to untrusted users until each compilation and execution is isolated in a stronger sandbox with resource, filesystem, process, and network restrictions. The existing timeout and output limits alone are not sufficient protection.