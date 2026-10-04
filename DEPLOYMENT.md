# Deploy JoeScript Studio

The frontend is a static Vercel site. The compiler API runs separately in the Linux container defined by the repository-root `Dockerfile`.

## Deploy the compiler API

The repository includes a Render Blueprint in `render.yaml`. After pushing it to GitHub, open Render's Blueprint creation flow, select this repository, and apply the blueprint. It creates a Docker web service named `joescript-api`, builds from the repository-root `Dockerfile`, and uses `/health` as its health check. The service listens on the platform-provided `PORT`.

When Render prompts for `ALLOWED_ORIGIN`, set it to the exact production Vercel origin, with no trailing slash, for example `https://joe-script-frontend.vercel.app`. For Vercel preview deployments, the origin must also be configured to match the preview URL being tested.

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