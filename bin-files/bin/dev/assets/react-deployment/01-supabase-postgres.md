# Supabase and PostgreSQL Setup for a React Contact Form

This guide recreates the Supabase portion of the portfolio deployment: running Supabase locally, creating the PostgreSQL schema through a migration, querying the data, exposing a safe Edge Function, connecting the React client, and sending an email notification with Resend.

The final request path is:

```text
React contact form
  -> Supabase JavaScript client
  -> submit-contact Edge Function
  -> validate request
  -> verify Turnstile (covered in Guide 3)
  -> insert into PostgreSQL
  -> send notification through Resend
```

The browser only receives Supabase's public project URL and publishable key. Database-admin credentials, the Resend key, and the Turnstile secret stay inside Supabase Edge Function secrets.

## 1. Install the project dependencies

From the React project root:

```bash
cd /home/dxle/dev/web-apps/portfolio
npm install @supabase/supabase-js
npm install --save-dev supabase
```

Why both packages exist:

- `@supabase/supabase-js` is imported by the React application.
- `supabase` provides the local-development, migration, secrets, and deployment CLI commands.

## 2. Initialize Supabase locally

```bash
npx supabase init
```

This creates the `supabase/` directory and its configuration. Commit migrations, function source, and `config.toml`; do not commit local secret files.

Start the local stack:

```bash
npx supabase start
```

Supabase local development uses Docker. On Arch Linux, Docker was installed and enabled with:

```bash
sudo pacman -S docker
sudo systemctl enable --now docker.service
sudo systemctl status docker.service --no-pager
docker info
```

If `docker info` reports permission denied for `/var/run/docker.sock`, permanently add the current user to the `docker` group:

```bash
sudo usermod -aG docker "$USER"
newgrp docker
id -nG
docker info
```

`newgrp docker` updates the current shell. Logging out and back in applies the group membership to all future sessions. Do not solve this by making the Docker socket world-writable.

The optional terminal dashboard used during setup was:

```bash
sudo pacman -S lazydocker
lazydocker
```

When the stack is running, inspect its URLs and development keys with:

```bash
npx supabase status
```

The default local endpoints normally include:

- API: `http://127.0.0.1:54321`
- PostgreSQL: `postgresql://postgres:postgres@127.0.0.1:54322/postgres`
- Studio: `http://127.0.0.1:54323`
- Mailpit: `http://127.0.0.1:54324`

These local credentials are development defaults. Do not reuse them as production credentials.

## 3. Create the database migration

The first attempted command was invalid:

```bash
npx supabase migration start new create_contact_messages
```

The correct command is:

```bash
npx supabase migration new create_contact_messages
```

Edit the generated file under `supabase/migrations/` and define the table through SQL:

```sql
create table public.contact_messages (
  id bigint generated always as identity primary key,
  sender_email text not null
    check (char_length(sender_email) between 3 and 254),
  message text not null
    check (char_length(btrim(message)) between 1 and 5000),
  status text not null default 'new'
    check (status in ('new', 'read', 'archived')),
  created_at timestamptz not null default now()
);

alter table public.contact_messages enable row level security;

revoke all on table public.contact_messages from anon, authenticated;
revoke all on sequence public.contact_messages_id_seq from anon, authenticated;

grant select, insert, update, delete
  on table public.contact_messages
  to service_role;

grant usage, select
  on sequence public.contact_messages_id_seq
  to service_role;
```

This design deliberately prevents browser clients from directly inserting, reading, updating, or deleting messages. Only trusted server-side code using the service role can access the table.

Apply every local migration from a clean database:

```bash
npx supabase db reset
```

This is destructive to the local Supabase database: it recreates the database and reapplies migrations. It does not reset the hosted production database. Inspect the target before running database-reset commands.

Open local Studio at `http://127.0.0.1:54323`, select Table Editor, and confirm that `public.contact_messages` exists.

## 4. Query contact messages

Use the SQL Editor in Supabase Studio or a PostgreSQL client connected to the database.

List newest submissions first:

```sql
select id, sender_email, message, status, created_at
from public.contact_messages
order by created_at desc;
```

List only unread messages:

```sql
select id, sender_email, message, created_at
from public.contact_messages
where status = 'new'
order by created_at desc;
```

Mark one message as read:

```sql
update public.contact_messages
set status = 'read'
where id = 123;
```

Archive one message:

```sql
update public.contact_messages
set status = 'archived'
where id = 123;
```

Before deleting a record, preview the exact row:

```sql
select *
from public.contact_messages
where id = 123;
```

Only after confirming the target should you run:

```sql
delete from public.contact_messages
where id = 123;
```

Database reads and writes from application code should remain in server-side functions. The browser's publishable client cannot bypass the table revokes or safely hold a service-role key.

## 5. Generate TypeScript database types

Create a shared function directory and generate types from the local schema:

```bash
mkdir -p supabase/functions/_shared
npx supabase gen types typescript --local > supabase/functions/_shared/database.types.ts
```

Regenerate this file whenever a migration changes the database schema. For hosted schema types, use the linked project only when intentionally synchronizing against production.

## 6. Create the Edge Function

The first command contained a spelling error and attempted to resolve a nonexistent npm package:

```bash
npx subabase functions new submit-contact
```

The correct command is:

```bash
npx supabase functions new submit-contact
```

The function lives at:

```text
supabase/functions/submit-contact/
├── deno.json
├── index.ts
├── send-contact-email.ts
└── verify-turnstile.ts
```

The last file is covered in the Turnstile guide.

The function-specific `deno.json` used by this project maps its server dependencies:

```json
{
  "imports": {
    "@supabase/functions-js": "jsr:@supabase/functions-js@^2",
    "@supabase/server": "npm:@supabase/server@^1"
  }
}
```

The corresponding function section in `supabase/config.toml` is:

```toml
[functions.submit-contact]
enabled = true
verify_jwt = false
import_map = "./functions/submit-contact/deno.json"
entrypoint = "./functions/submit-contact/index.ts"
```

`verify_jwt = false` is intentional because anonymous visitors must be able to submit the public contact form. It does not mean the function trusts the request: the function validates the method, JSON shape, lengths, and Turnstile token before performing the database insert.

## 7. Keep validation modular

Treat incoming JSON as `unknown`, narrow it, trim it, and enforce the same limits as PostgreSQL. A reusable validator can look like this:

```ts
type ContactSubmission = {
  senderEmail: string;
  message: string;
  turnstileToken: string;
};

type ValidationResult =
  | { valid: true; submission: ContactSubmission }
  | { valid: false; message: string };

const limits = {
  emailLength: 254,
  messageLength: 5000,
  turnstileTokenLength: 2048,
};

function validateSubmission(body: unknown): ValidationResult {
  if (!body || typeof body !== "object") {
    return { valid: false, message: "Invalid request body" };
  }

  const record = body as Record<string, unknown>;
  const senderEmail =
    typeof record.senderEmail === "string"
      ? record.senderEmail.trim()
      : "";
  const message =
    typeof record.message === "string" ? record.message.trim() : "";
  const turnstileToken =
    typeof record.turnstileToken === "string"
      ? record.turnstileToken
      : "";
  const emailPattern = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

  if (
    !senderEmail ||
    senderEmail.length > limits.emailLength ||
    !emailPattern.test(senderEmail)
  ) {
    return { valid: false, message: "Invalid email address" };
  }

  if (!message || message.length > limits.messageLength) {
    return { valid: false, message: "Invalid message" };
  }

  if (
    !turnstileToken ||
    turnstileToken.length > limits.turnstileTokenLength
  ) {
    return { valid: false, message: "Invalid verification token" };
  }

  return {
    valid: true,
    submission: { senderEmail, message, turnstileToken },
  };
}
```

Client-side checks are useful for feedback, but this server-side validation is the security boundary.

## 8. Insert the row and return it

Inside the `submit-contact` handler, verify the HTTP method, parse JSON, validate it, verify Turnstile, and only then insert:

```ts
export default {
  fetch: withSupabase<Database>(
    { auth: "publishable" },
    async (request, context) => {
      if (request.method !== "POST") {
        return Response.json(
          { message: "Method not allowed" },
          { status: 405, headers: { Allow: "POST" } },
        );
      }

      let body: unknown;

      try {
        body = await request.json();
      } catch {
        return Response.json(
          { message: "Invalid JSON" },
          { status: 400 },
        );
      }

      const validation = validateSubmission(body);

      if (!validation.valid) {
        return Response.json(
          { message: validation.message },
          { status: 400 },
        );
      }

      const turnstileValid = await verifyTurnstileToken(
        validation.submission.turnstileToken,
      );

      if (!turnstileValid) {
        return Response.json(
          { message: "Verification failed." },
          { status: 403 },
        );
      }

      const { data: contactMessage, error } = await context.supabaseAdmin
        .from("contact_messages")
        .insert({
          sender_email: validation.submission.senderEmail,
          message: validation.submission.message,
        })
        .select("id, sender_email, message, created_at")
        .single();

      if (error || !contactMessage) {
        console.error("Contact submission failed:", error);

        return Response.json(
          { message: "Unable to submit message." },
          { status: 500 },
        );
      }

      let notificationSent = true;

      try {
        await sendContactEmail(contactMessage);
      } catch (emailError) {
        notificationSent = false;
        console.error("Contact email failed:", emailError);
      }

      return Response.json(
        { success: true, notificationSent },
        { status: 201 },
      );
    },
  ),
};
```

The email notification happens after a successful insert. If Resend is temporarily unavailable, the visitor's message remains stored in PostgreSQL and the response reports `notificationSent: false` rather than losing the submission.

## 9. Send the notification through Resend

Create a separate `send-contact-email.ts` module:

```ts
import "@supabase/functions-js/edge-runtime.d.ts";

type ContactMessage = {
  id: number;
  sender_email: string;
  message: string;
  created_at: string;
};

function getRequiredEnvironmentVariable(name: string) {
  const value = Deno.env.get(name);

  if (!value) {
    throw new Error(`Missing environment variable: ${name}`);
  }

  return value;
}

export async function sendContactEmail(
  contactMessage: ContactMessage,
) {
  const apiKey = getRequiredEnvironmentVariable("RESEND_API_KEY");
  const recipient = getRequiredEnvironmentVariable("CONTACT_EMAIL_TO");
  const sender = getRequiredEnvironmentVariable("CONTACT_EMAIL_FROM");

  const response = await fetch("https://api.resend.com/emails", {
    method: "POST",
    headers: {
      Authorization: `Bearer ${apiKey}`,
      "Content-Type": "application/json",
      "User-Agent": "dxle-portfolio/1.0",
      "Idempotency-Key": `contact-message/${contactMessage.id}`,
    },
    body: JSON.stringify({
      from: sender,
      to: [recipient],
      reply_to: contactMessage.sender_email,
      subject: "New portfolio contact message",
      text: [
        `From: ${contactMessage.sender_email}`,
        `Received: ${contactMessage.created_at}`,
        "",
        contactMessage.message,
      ].join("\n"),
    }),
  });

  if (!response.ok) {
    const responseBody = await response.text();
    throw new Error(
      `Resend failed (${response.status}): ${responseBody}`,
    );
  }
}
```

The `from` address must be allowed by Resend, normally through a verified domain or verified sender. `reply_to` is the visitor's email so replying from your inbox addresses the visitor. The database row ID provides an idempotency key, helping prevent duplicate notification emails when the same stored message is retried.

## 10. Configure local and hosted function secrets

Create `supabase/functions/.env.local` for local function development:

```dotenv
RESEND_API_KEY=<resend-api-key>
CONTACT_EMAIL_TO=<your-inbox@example.com>
CONTACT_EMAIL_FROM=<verified-sender@example.com>
TURNSTILE_SECRET_KEY=<turnstile-secret-key>
```

For a separate production-upload source, use `supabase/functions/.env.production.local` with the hosted values. Both files must remain ignored by Git.

Check before staging anything:

```bash
git check-ignore -v supabase/functions/.env.local
git check-ignore -v supabase/functions/.env.production.local
```

Start the local function with its local secrets:

```bash
npx supabase functions serve submit-contact \
  --env-file supabase/functions/.env.local
```

Upload production secrets to the linked hosted project:

```bash
npx supabase secrets set \
  --env-file supabase/functions/.env.production.local \
  --project-ref <project-ref>
```

List secret names and digests without revealing their values:

```bash
npx supabase secrets list --project-ref <project-ref>
```

Never put `RESEND_API_KEY`, `SUPABASE_SERVICE_ROLE_KEY`, or `TURNSTILE_SECRET_KEY` in a `VITE_*` variable. Vite variables are compiled into public browser JavaScript.

## 11. Connect the React application

Add browser-safe values to the project's `.env.local`:

```dotenv
VITE_SUPABASE_URL=http://127.0.0.1:54321
VITE_SUPABASE_PUBLISHABLE_KEY=<local-publishable-key>
```

For hosted builds, use the hosted project URL and hosted publishable key. The project reference is the subdomain in:

```text
https://<project-ref>.supabase.co
```

It is also visible in the Supabase dashboard URL and with:

```bash
npx supabase projects list
```

Create one client module, such as `src/lib/supabase.ts`:

```ts
import { createClient } from "@supabase/supabase-js";

const supabaseUrl = import.meta.env.VITE_SUPABASE_URL;
const supabasePublishableKey =
  import.meta.env.VITE_SUPABASE_PUBLISHABLE_KEY;

if (!supabaseUrl || !supabasePublishableKey) {
  throw new Error("Missing Supabase environment variables");
}

export const supabase = createClient(
  supabaseUrl,
  supabasePublishableKey,
);
```

Keep the network call in a service module such as `src/services/contact.ts`:

```ts
import { supabase } from "../lib/supabase.ts";

export type ContactSubmission = {
  senderEmail: string;
  message: string;
  turnstileToken: string;
};

export async function submitContact(
  submission: ContactSubmission,
) {
  const { data, error } = await supabase.functions.invoke(
    "submit-contact",
    { body: submission },
  );

  if (error) {
    throw error;
  }

  return data;
}
```

The dialog should call `submitContact()` from its form submit handler. Keep the dialog concerned with fields, loading state, errors, and alerts while the service owns the Supabase request.

## 12. Test locally

Run the React application:

```bash
npm run dev
```

Run the local Edge Function in another terminal:

```bash
npx supabase functions serve submit-contact \
  --env-file supabase/functions/.env.local
```

Before Turnstile was added, the function was tested directly with:

```bash
curl -i \
  --request POST \
  'http://127.0.0.1:54321/functions/v1/submit-contact' \
  --header 'apikey: <local-publishable-key>' \
  --header 'Content-Type: application/json' \
  --data '{"senderEmail":"test@example.com","message":"Hello from the local function"}'
```

Once Turnstile is required, a raw curl request also needs a valid Turnstile token. Those tokens are short-lived and single-use, so the most realistic integration test is the rendered contact form.

Confirm all three outcomes:

1. The browser receives HTTP `201` and the success alert.
2. Supabase Studio shows a new `contact_messages` row.
3. Resend delivers the notification email to the configured inbox.

## 13. Link and deploy the hosted Supabase project

Authenticate and inspect projects:

```bash
npx supabase login
npx supabase projects list
```

Link this local directory:

```bash
npx supabase link --project-ref <project-ref>
```

Preview the database migration operation first:

```bash
npx supabase db push --dry-run
```

Apply the migration after reviewing the dry run:

```bash
npx supabase db push
```

Upload secrets, then deploy the function:

```bash
npx supabase secrets set \
  --env-file supabase/functions/.env.production.local \
  --project-ref <project-ref>

npx supabase functions deploy submit-contact \
  --project-ref <project-ref>
```

The hosted function was tested before Turnstile with this request shape:

```bash
curl -i \
  --request POST \
  'https://<project-ref>.supabase.co/functions/v1/submit-contact' \
  --header 'apikey: <hosted-publishable-key>' \
  --header 'Content-Type: application/json' \
  --data '{"senderEmail":"test@example.com","message":"Hello from production"}'
```

After enabling Turnstile, submit through the hosted page so the request includes a real token.

## 14. How to verify production data

In the hosted Supabase dashboard:

1. Open Table Editor.
2. Select the `public` schema.
3. Open `contact_messages`.
4. Sort by `created_at` descending.

You can also use the hosted SQL Editor with the read query from section 4. The React browser client should not be granted read access to private contact messages.

## 15. Command ledger from this setup

These are the relevant commands used during the Supabase and email journey, in practical order:

```bash
npm install @supabase/supabase-js
npm install --save-dev supabase
npx supabase init
npx supabase start
sudo pacman -S docker
sudo systemctl enable --now docker.service
sudo systemctl status docker.service --no-pager
docker info
sudo usermod -aG docker "$USER"
newgrp docker
id -nG
docker info
sudo pacman -S lazydocker
lazydocker
npx supabase status
npx supabase migration new create_contact_messages
npx supabase db reset
npx supabase functions new submit-contact
mkdir -p supabase/functions/_shared
npx supabase gen types typescript --local > supabase/functions/_shared/database.types.ts
npx supabase functions serve submit-contact
npx supabase login
npx supabase projects list
npx supabase link --project-ref <project-ref>
npx supabase db push --dry-run
npx supabase db push
npx supabase secrets set --env-file supabase/functions/.env.local
npx supabase secrets list
npx supabase functions serve submit-contact --env-file supabase/functions/.env.local
npx supabase secrets set --env-file supabase/functions/.env.production.local --project-ref <project-ref>
npx supabase secrets list --project-ref <project-ref>
npx supabase functions deploy submit-contact --project-ref <project-ref>
npm run dev
```

The invalid commands encountered were `npx supabase migration start new ...` and `npx subabase functions new ...`. Keep them only as troubleshooting history; do not reuse them.

## 16. Final safety checklist

- Migrations are committed and reproducible.
- RLS is enabled on `contact_messages`.
- `anon` and `authenticated` have no direct table access.
- Only the Edge Function inserts messages.
- The function validates unknown input and enforces length limits.
- Turnstile verification happens before database or email work.
- `.env.local` and `.env.production.local` are ignored by Git.
- The service-role, Resend, and Turnstile secret values never use a `VITE_` prefix.
- `npx supabase db push --dry-run` is reviewed before a production push.
- A successful insert is retained even if notification email delivery fails.

## References

- [Supabase CLI getting started](https://supabase.com/docs/guides/local-development/cli/getting-started)
- [Supabase local development workflow](https://supabase.com/docs/guides/local-development/cli-workflows)
- [Supabase database migrations](https://supabase.com/docs/guides/local-development/database-migrations)
- [Supabase Edge Functions quickstart](https://supabase.com/docs/guides/functions/quickstart)
- [Supabase Edge Function secrets](https://supabase.com/docs/guides/functions/secrets)
- [Supabase JavaScript function invocation](https://supabase.com/docs/reference/javascript/v1/functions-invoke)
- [Resend idempotency keys](https://resend.com/docs/dashboard/emails/idempotency-keys)
