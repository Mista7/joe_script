import { copyFile, mkdir, writeFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const frontendDirectory = dirname(fileURLToPath(import.meta.url));
const outputDirectory = join(frontendDirectory, 'dist');
const apiUrl = process.env.JOESCRIPT_API_URL?.trim() || '';

if (apiUrl) {
  const parsedUrl = new URL(apiUrl);
  if (!['http:', 'https:'].includes(parsedUrl.protocol) || parsedUrl.pathname !== '/' || parsedUrl.search || parsedUrl.hash) {
    throw new Error('JOESCRIPT_API_URL must be the backend origin, such as https://joescript-api.example.com');
  }
}

await mkdir(outputDirectory, { recursive: true });
for (const filename of ['index.html', 'styles.css', 'app.js']) {
  await copyFile(join(frontendDirectory, filename), join(outputDirectory, filename));
}
await writeFile(
  join(outputDirectory, 'config.js'),
  `window.JOESCRIPT_API_URL = ${JSON.stringify(apiUrl.replace(/\/$/, ''))};\n`,
);
process.stdout.write(`Built JoeScript Studio in ${outputDirectory}\n`);