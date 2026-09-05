# Hosting a React and Vite Site on Cloudflare Pages

This guide recreates the Cloudflare Pages deployment for the portfolio. It covers choosing the correct Cloudflare workflow, configuring a GitHub-backed Vite build, supplying public build variables, resolving the npm peer-dependency failure encountered during deployment, and verifying automatic deployments.

The deployed architecture is:

```text
GitHub main branch
  -> Cloudflare Pages build
  -> npm install
  -> npm run build
  -> publish dist/
  -> static portfolio on *.pages.dev

Browser contact form
  -> hosted Supabase Edge Function
  -> PostgreSQL and Resend
```

Cloudflare Pages hosts the React frontend. The Supabase Edge Function remains deployed and executed by Supabase.

## 1. Confirm the project builds locally

From the repository root:

```bash
cd /home/dxle/dev/web-apps/portfolio
npm install
npm run lint
npm run build
```

For this Vite project, a successful build creates `dist/`. Cloudflare must publish that directory.

The bundle-size message about a chunk larger than 500 kB is a warning, not a build failure. Code splitting can be handled later; deployment can proceed when the command exits successfully.

Before deployment, also inspect the working tree:

```bash
git status --short
```

Commit only the intended project files. Do not commit `.env.local`, Supabase function secret files, or service-role credentials.

## 2. Choose Cloudflare Pages, not a standalone Worker

In the Cloudflare dashboard:

1. Open **Compute**.
2. Open **Workers & Pages**.
3. Choose **Create application**.
4. On the screen offering GitHub, GitLab, templates, and static uploads, use **Continue to Pages** if Cloudflare directs GitHub setup into the Workers workflow.
5. Connect GitHub and select the repository.

The wrong screen in this journey asked for both:

```text
Build command: npm run build
Deploy command: npx wrangler deploy
```

That is the Worker-oriented application flow. A plain static React/Vite site does not need `npx wrangler deploy` in this Git-connected Pages setup.

The correct Pages screen is titled **Set up builds and deployments** and asks for a build output directory.

## 3. Configure the Pages build

Use these values:

| Setting | Value |
| --- | --- |
| Project name | `portfolio` or another available name |
| Production branch | `main` |
| Framework preset | `None` or the Vite preset if available |
| Build command | `npm run build` |
| Build output directory | `dist` |
| Root directory | Leave blank when `package.json` is at repository root |

The generated URL has this form:

```text
https://<pages-project>.pages.dev
```

Preview deployments use separate generated hostnames. Production deployments follow the configured production branch.

## 4. Add public build variables

Open the Pages project, then go to:

```text
Settings -> Variables and secrets
```

Add these as build-time text variables:

```dotenv
VITE_SUPABASE_URL=https://<project-ref>.supabase.co
VITE_SUPABASE_PUBLISHABLE_KEY=<supabase-publishable-key>
VITE_TURNSTILE_SITE_KEY=<turnstile-site-key>
```

These are intentionally public:

- The Supabase URL identifies the hosted project.
- The Supabase publishable key is designed for browser use when database authorization is correctly enforced.
- The Turnstile site key identifies the public widget.

Never add these private values to the Pages frontend environment:

```text
SUPABASE_SERVICE_ROLE_KEY
RESEND_API_KEY
TURNSTILE_SECRET_KEY
CONTACT_EMAIL_FROM
CONTACT_EMAIL_TO
```

Vite replaces `import.meta.env.VITE_*` during the build, so the final JavaScript contains those values. Changing a Pages variable requires a new deployment before the frontend sees the new value.

## 5. Deploy from GitHub

If the current branch is ready:

```bash
git status --short
git add <specific-files>
git commit -m "configure production deployment"
git push origin main
```

Cloudflare automatically clones the repository, installs dependencies, runs `npm run build`, and publishes `dist/`.

During the original journey, broad staging was occasionally performed with:

```bash
git add .
```

Prefer targeted `git add` paths in future work so unrelated files and local configuration are not accidentally included.

## 6. Resolve the ESLint peer-dependency build failure

The first Pages build failed during dependency installation with an npm `ERESOLVE` error. The conflict was:

```text
eslint-plugin-react@7.37.5
  expects ESLint through the supported 9.x range

the project
  used ESLint 10.x
```

Because the project's ESLint configuration did not use `eslint-plugin-react`, the safe fix was to remove the unused incompatible dependency:

```bash
npm uninstall --save-dev eslint-plugin-react
npm run lint
npm run build
```

Then commit both dependency files and push:

```bash
git add package.json package-lock.json
git commit -m "remove incompatible eslint plugin"
git push origin main
```

The actual project history recorded this repair in the `fixed some eslint conflicts` commit. After the push, the Pages build completed successfully.

Do not use `npm install --force` or `--legacy-peer-deps` as the permanent fix when an unused conflicting dependency can be removed. Those flags can hide a dependency graph that local and production environments resolve differently.

The same dependency was removed from reusable template files so future React projects would not inherit the conflict. To audit templates:

```bash
rg -n 'eslint-plugin-react' /home/dxle/bin/dev/assets/template-files
```

If a template still contains it, update the relevant `package.json` and regenerate that template's lock file in its own project context. Do not hand-edit `package-lock.json`.

## 7. Read the build log effectively

Cloudflare's deployment page separates the process into:

1. Initializing the build environment.
2. Cloning the Git repository.
3. Building the application.
4. Deploying to Cloudflare's network.

If step 3 fails, scroll to the first npm, TypeScript, or Vite error. The last lines normally report only that the build command exited with status 1; the actionable cause appears earlier.

Reproduce the same build locally:

```bash
npm install
npm run lint
npm run build
```

If `npm install` behaves differently locally, verify that `package-lock.json` is committed and current:

```bash
git status --short package.json package-lock.json
git diff -- package.json package-lock.json
```

## 8. Verify a successful deployment

In the Cloudflare Pages project:

1. Open **Deployments**.
2. Confirm the latest `main` deployment has a green status.
3. Open the production `*.pages.dev` URL.
4. Test normal navigation and static assets.
5. Open the email dialog and submit a real test message.

For the contact form, confirm the full hosted path:

1. Turnstile completes in the browser.
2. The form shows its success alert.
3. A new row appears in hosted Supabase.
4. The email reaches the configured inbox.

This project passed the end-to-end check, including repeated production submissions and email delivery.

## 9. Automatic deployment behavior

With Git integration enabled:

- A push to `main` starts a production deployment.
- A push to another enabled branch creates a preview deployment.
- Each deployment is associated with a specific commit.
- A failed build leaves the last successful production deployment active.

Before pushing a deployment-triggering commit, run:

```bash
npm run lint
npm run build
git status --short
```

## 10. Add a custom domain when ready

The site can remain on `*.pages.dev`, or a domain managed by Cloudflare can be attached:

1. Open the Pages project.
2. Select **Custom domains**.
3. Choose **Set up a custom domain**.
4. Enter the intended hostname, such as `portfolio.example.com` or the apex domain.
5. Review the DNS records Cloudflare proposes.
6. Confirm the change only when the exact hostname is correct.

DNS changes affect external traffic. Inspect the proposed record and preserve unrelated records before confirming it.

## 11. Common failures

### The build finishes but the site returns 404

Confirm the output directory is exactly:

```text
dist
```

Do not use `/dist`, `build`, or the repository root for a standard Vite build.

### Static assets are missing

Check that Vite produces the assets locally and that no hard-coded local filesystem paths are used:

```bash
npm run build
find dist -maxdepth 2 -type f | sort
```

### Environment variables are undefined

Check all three conditions:

1. The variable name starts with `VITE_`.
2. It is configured for the correct Pages environment.
3. A new deployment occurred after the variable was added or changed.

### The deployed contact form calls localhost

Set:

```dotenv
VITE_SUPABASE_URL=https://<project-ref>.supabase.co
```

The production site cannot reach the developer machine's `127.0.0.1` Supabase stack.

### A secret was accidentally committed

Removing it in a later commit is not enough because Git history retains it. Rotate the credential at its provider immediately, remove it from the repository, and then decide whether history rewriting is required. History rewriting is destructive and should only be done deliberately.

## 12. Command ledger from this deployment

```bash
cd /home/dxle/dev/web-apps/portfolio
npm install
npm run lint
npm run build
git status --short
npm uninstall --save-dev eslint-plugin-react
npm run lint
npm run build
git add package.json package-lock.json
git commit -m "remove incompatible eslint plugin"
git push origin main
rg -n 'eslint-plugin-react' /home/dxle/bin/dev/assets/template-files
```

Feature work for Supabase and Turnstile was also deployed through normal Git commits and pushes:

```bash
git add <specific-files>
git commit -m "configure supabase contact form"
git push origin main

git add <specific-files>
git commit -m "protect contact form with Turnstile"
git push origin main
```

## 13. Final deployment checklist

- `npm run lint` passes locally.
- `npm run build` passes locally and creates `dist/`.
- `package-lock.json` is committed.
- Pages uses the `main` production branch.
- Build command is `npm run build`.
- Output directory is `dist`.
- Root directory is blank for a repository-root Vite project.
- Only browser-safe `VITE_*` values are stored in Pages.
- Supabase and Turnstile private secrets remain in Supabase.
- The latest Cloudflare deployment is green.
- The hosted form creates a database row and sends an email.

## References

- [Cloudflare Pages Git integration](https://developers.cloudflare.com/pages/get-started/git-integration/)
- [Cloudflare Pages build configuration](https://developers.cloudflare.com/pages/configuration/build-configuration/)
