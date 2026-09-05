# Protecting a React Contact Form with Cloudflare Turnstile

This guide recreates the Turnstile integration for the portfolio contact form. Turnstile is Cloudflare's CAPTCHA alternative: most visitors are verified without solving a puzzle, but the browser still obtains a signed token and the server must validate that token before accepting the request.

The protected request path is:

```text
Turnstile widget in React
  -> short-lived, single-use token
  -> submitContact({ senderEmail, message, turnstileToken })
  -> Supabase submit-contact Edge Function
  -> Cloudflare Siteverify API
  -> PostgreSQL insert
  -> Resend notification
```

Rendering the widget alone does not protect the endpoint. Server-side Siteverify validation is mandatory.

## 1. Create a Turnstile widget

In the Cloudflare dashboard:

1. Open **Turnstile** under application security.
2. Choose **Add widget**.
3. Give it a descriptive name such as `portfolio-contact`.
4. Add the production hostname, such as the Pages `*.pages.dev` hostname and later the custom domain.
5. Choose the managed widget mode unless the application has a reason to force a visible challenge.
6. Create the widget.

Cloudflare provides two different values:

| Value | Where it belongs | Public? |
| --- | --- | --- |
| Site key | React/Vite frontend | Yes |
| Secret key | Supabase Edge Function secrets | No |

Never put the secret key into a `VITE_*` environment variable.

## 2. Install the React Turnstile component

From the portfolio root:

```bash
npm install --save-exact @marsidev/react-turnstile
```

The exact version is stored in `package.json` and `package-lock.json`, keeping local and Cloudflare builds reproducible.

## 3. Configure the frontend site key

For local Vite development, add this to `.env.local`:

```dotenv
VITE_TURNSTILE_SITE_KEY=<turnstile-site-key>
```

For the hosted site, add the same variable to:

```text
Cloudflare Pages -> portfolio -> Settings -> Variables and secrets
```

Use a text variable named:

```text
VITE_TURNSTILE_SITE_KEY
```

After adding or changing it, trigger a new Pages build because Vite embeds `VITE_*` values at build time.

## 4. Extend the frontend submission type

The contact service must accept and forward the token:

```ts
export type ContactSubmission = {
  senderEmail: string;
  message: string;
  turnstileToken: string;
};
```

The invocation remains modular:

```ts
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

If TypeScript reports that `turnstileToken` does not exist in `ContactSubmission`, the object passed by the dialog has been updated but this shared type has not. Add the field to the service type rather than suppressing the error.

## 5. Add Turnstile state to the email dialog

Import the component and its instance type:

```ts
import {
  Turnstile,
  type TurnstileInstance,
} from "@marsidev/react-turnstile";
import {
  useRef,
  useState,
  type SubmitEvent as ReactSubmitEvent,
} from "react";
```

Inside `EmailDialog`, create the token state, widget ref, and site-key lookup:

```ts
const turnstileRef = useRef<TurnstileInstance | null>(null);
const [turnstileToken, setTurnstileToken] = useState<string | null>(
  null,
);
const turnstileSiteKey = import.meta.env.VITE_TURNSTILE_SITE_KEY;
```

Fail clearly when the public site key is missing. This catches a misconfigured Pages build early:

```ts
if (!turnstileSiteKey) {
  throw new Error("Missing VITE_TURNSTILE_SITE_KEY");
}
```

## 6. Render the widget in the form

Place the widget inside the actual `<form>`, above the submit button:

```tsx
<Turnstile
  ref={turnstileRef}
  siteKey={turnstileSiteKey}
  onSuccess={setTurnstileToken}
  onExpire={() => setTurnstileToken(null)}
  onError={() => setTurnstileToken(null)}
  options={{
    action: "contact",
    theme: "light",
    size: "flexible",
  }}
/>
```

The `action` value is checked again by the Edge Function, which prevents a token created for a different widget action from being accepted for contact submissions.

Use an actual form element:

```tsx
<form onSubmit={handleSubmit}>
  {/* email input, message textarea, Turnstile, submit button */}
</form>
```

A surrounding `<div>` is fine for layout, but it cannot replace the `<form>` if you rely on submit behavior, `FormData`, Enter-key submission, and form reset.

## 7. Submit the token with the form

The submit handler should read the form fields, require a token, call the service, and reset the widget whether the request succeeds or fails:

```ts
async function handleSubmit(
  event: ReactSubmitEvent<HTMLFormElement>,
) {
  event.preventDefault();

  const form = event.currentTarget;
  const formData = new FormData(form);
  const senderEmail = formData.get("senderEmail");
  const message = formData.get("message");

  if (
    typeof senderEmail !== "string" ||
    typeof message !== "string" ||
    !turnstileToken
  ) {
    setSubmissionStatus("error");
    setSubmissionMessage("Please complete the verification.");
    return;
  }

  setSubmissionStatus("submitting");
  setSubmissionMessage("");

  try {
    await submitContact({
      senderEmail,
      message,
      turnstileToken,
    });

    form.reset();
    setSubmissionStatus("success");
    setShowSentAlert(true);
  } catch (error) {
    setSubmissionStatus("error");
    setSubmissionMessage(
      error instanceof Error
        ? error.message
        : "Unable to send your message.",
    );
  } finally {
    setTurnstileToken(null);
    turnstileRef.current?.reset();
  }
}
```

In current React typings, use React's `SubmitEvent` alias for a form submit handler. The browser DOM `SubmitEvent` type is not generic, which is why `SubmitEvent<HTMLFormElement>` without the React import produced `Type 'SubmitEvent' is not generic`.

Disable the submit button while submitting or while no valid token is available:

```tsx
<button
  type="submit"
  disabled={submissionStatus === "submitting" || !turnstileToken}
>
  {submissionStatus === "submitting" ? "Sending..." : "Send"}
</button>
```

The text inputs can remain editable before Turnstile succeeds. Only submission needs to be blocked.

## 8. Reset Turnstile when the dialog closes

Turnstile tokens expire and are single-use. Reset token-related state whenever the dialog closes so reopening it cannot reuse a stale success state:

```ts
function handleOpenChange(nextOpen: boolean) {
  if (!nextOpen) {
    setShowSentAlert(false);
    setSubmissionStatus("idle");
    setSubmissionMessage("");
    setTurnstileToken(null);
    turnstileRef.current?.reset();
  }

  onOpenChange(nextOpen);
}
```

This also fixes the earlier behavior where a dialog-local notification remained visible after closing and reopening the dialog.

## 9. Store the Turnstile secret in Supabase

Add the secret only to the local file used for Supabase function secrets:

```dotenv
TURNSTILE_SECRET_KEY=<turnstile-secret-key>
```

The production upload file used during this journey was:

```text
supabase/functions/.env.production.local
```

Confirm that Git ignores it:

```bash
git check-ignore -v supabase/functions/.env.production.local
```

Upload it to the hosted Supabase project:

```bash
npx supabase secrets set \
  --env-file supabase/functions/.env.production.local \
  --project-ref <project-ref>
```

Confirm the secret name exists without printing its value:

```bash
npx supabase secrets list --project-ref <project-ref>
```

The `DIGEST` displayed by the CLI is expected; Supabase does not print secret values back to the terminal.

## 10. Validate the token in the Edge Function

Create `supabase/functions/submit-contact/verify-turnstile.ts`:

```ts
type TurnstileResult = {
  success: boolean;
  action?: string;
  "error-codes"?: string[];
};

export async function verifyTurnstileToken(
  token: string,
): Promise<boolean> {
  const secretKey = Deno.env.get("TURNSTILE_SECRET_KEY");

  if (!secretKey) {
    throw new Error("Missing TURNSTILE_SECRET_KEY");
  }

  const response = await fetch(
    "https://challenges.cloudflare.com/turnstile/v0/siteverify",
    {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
      },
      body: new URLSearchParams({
        secret: secretKey,
        response: token,
      }),
      signal: AbortSignal.timeout(10_000),
    },
  );

  if (!response.ok) {
    throw new Error(
      `Turnstile verification failed with ${response.status}`,
    );
  }

  const result = (await response.json()) as TurnstileResult;

  return result.success && result.action === "contact";
}
```

The `Deno` global is available in Supabase's Edge Runtime. If an editor says `Cannot find name 'Deno'`, ensure the function has its generated Deno configuration and runtime types. A type-only editor warning should not be solved by moving the secret into React.

For a Supabase Edge Function module that needs runtime declarations, this side-effect import is valid:

```ts
import "@supabase/functions-js/edge-runtime.d.ts";
```

If the editor cannot resolve that import, open the repository as a Deno-aware workspace or keep the import in the function subtree governed by its `deno.json`; do not install secret-handling code into the browser application.

## 11. Verify before inserting into PostgreSQL

Extend the server-side `ContactSubmission` type and validator with:

```ts
turnstileToken: string;
```

After normal input validation and before the database insert, call the helper:

```ts
let turnstileValid: boolean;

try {
  turnstileValid = await verifyTurnstileToken(
    validation.submission.turnstileToken,
  );
} catch (error) {
  console.error("Turnstile verification unavailable:", error);

  return Response.json(
    { message: "Verification unavailable." },
    { status: 503 },
  );
}

if (!turnstileValid) {
  return Response.json(
    { message: "Verification failed." },
    { status: 403 },
  );
}
```

Only after this block should the function call:

```ts
context.supabaseAdmin.from("contact_messages").insert(...)
```

This order matters. Rejecting the bot after insertion would still fill the database and could still trigger email spam.

## 12. Use a safe deployment order

There are two independently deployed pieces:

- The React frontend on Cloudflare Pages.
- The `submit-contact` backend on Supabase.

A safe rollout is:

1. Add the Turnstile widget in Cloudflare and record the site and secret keys.
2. Add `VITE_TURNSTILE_SITE_KEY` to Cloudflare Pages.
3. Add `TURNSTILE_SECRET_KEY` to Supabase secrets.
4. Deploy the Supabase function that verifies tokens.
5. Push the React frontend that supplies tokens.
6. Test the production form.

The function was deployed with:

```bash
npx supabase functions deploy submit-contact \
  --project-ref <project-ref>
```

The frontend was checked and pushed with:

```bash
npm run lint
npm run build
git add <specific-turnstile-files>
git commit -m "protect contact form with Turnstile"
git push origin main
```

A later visual change switched the widget to light mode and was deployed with:

```bash
git add src/components/widgets/EmailDialog.tsx
git commit -m "switch Turnstile to light mode"
git push origin main
```

## 13. Test the protection

### Normal production test

1. Open the production Pages URL.
2. Open the email dialog.
3. Wait for Turnstile to complete.
4. Submit a unique test message.
5. Confirm the success alert.
6. Confirm one new PostgreSQL row.
7. Confirm one Resend email.

### Missing-token test

In browser developer tools, try invoking the function without `turnstileToken`. The server should return a `400` validation response and create no database row.

### Invalid-token test

Send a made-up token. Siteverify should reject it, the function should return `403`, and neither the insert nor email should occur.

### Replay test

A successfully validated token should not work again. Reset the widget after every attempt so the frontend obtains a fresh token.

### Availability test

If Siteverify cannot be reached, the function should fail closed with `503`. It should not insert the message while verification is unavailable.

Cloudflare provides documented test keys for automated and local tests. Use the official test-key table rather than a production secret in fixtures or CI.

## 14. Inspect Turnstile behavior

Use both providers when debugging:

- Cloudflare Turnstile analytics show widget challenges and validation behavior.
- Supabase Edge Function logs show validation failures, response codes, and backend exceptions.

Do not log the full Turnstile token or secret. Useful logs identify the stage and error category without exposing credentials.

## 15. Rotate an exposed secret immediately

If the Turnstile secret appears in a screenshot, terminal recording, Git commit, or public paste:

1. Open Cloudflare Turnstile.
2. Select the widget.
3. Open its settings.
4. Rotate the secret key.
5. Put the new key in `supabase/functions/.env.production.local`.
6. Upload it again:

```bash
npx supabase secrets set \
  --env-file supabase/functions/.env.production.local \
  --project-ref <project-ref>
```

7. Confirm the secret digest changed:

```bash
npx supabase secrets list --project-ref <project-ref>
```

8. Test the production contact form again.

Cloudflare's rotation flow can temporarily allow both the old and new secret depending on the selected invalidation option. Choose immediate invalidation when the old secret was publicly exposed, while understanding that requests using the old secret will stop working immediately.

## 16. Spam protection beyond Turnstile

Turnstile is the main bot-verification layer, not a complete abuse-prevention system. For a public contact endpoint, also consider:

- A server-side rate limit per IP or hashed client identifier.
- A maximum message size, already enforced at 5,000 characters.
- Server-side email validation and trimming.
- A honeypot field for basic automated form fillers.
- Resend idempotency keys to prevent duplicate email delivery.
- Logging failure categories and monitoring sudden volume increases.
- A database status workflow so messages can be reviewed and archived.

Do not rely only on disabling the frontend button. Attackers can call the public Edge Function directly.

## 17. Command ledger from this setup

```bash
cd /home/dxle/dev/web-apps/portfolio
npm install --save-exact @marsidev/react-turnstile
npm run lint
npm run build
git check-ignore -v supabase/functions/.env.production.local
npx supabase secrets set --env-file supabase/functions/.env.production.local --project-ref <project-ref>
npx supabase secrets list --project-ref <project-ref>
npx supabase functions deploy submit-contact --project-ref <project-ref>
git add <specific-turnstile-files>
git commit -m "protect contact form with Turnstile"
git push origin main
git add src/components/widgets/EmailDialog.tsx
git commit -m "switch Turnstile to light mode"
git push origin main
```

For local function testing with the local secrets file:

```bash
npx supabase functions serve submit-contact \
  --env-file supabase/functions/.env.local
```

## 18. Final security checklist

- The site key is stored as `VITE_TURNSTILE_SITE_KEY` in the frontend.
- The secret is stored as `TURNSTILE_SECRET_KEY` only in Supabase.
- Both local secret files are ignored by Git.
- The React service sends `turnstileToken` in the request body.
- The Edge Function validates the token with Siteverify.
- The server checks `action === "contact"`.
- Verification occurs before database insertion and email sending.
- The widget resets after success, error, expiry, and dialog close.
- The submit button remains disabled without a valid token.
- Missing, invalid, replayed, and unavailable-verification cases fail closed.
- Exposed secrets are rotated, uploaded, and production-tested again.

## References

- [Cloudflare Turnstile get started](https://developers.cloudflare.com/turnstile/get-started/)
- [Turnstile client-side rendering](https://developers.cloudflare.com/turnstile/get-started/client-side-rendering/)
- [Turnstile widget configuration](https://developers.cloudflare.com/turnstile/get-started/client-side-rendering/widget-configurations/)
- [Turnstile server-side validation](https://developers.cloudflare.com/turnstile/get-started/server-side-validation/)
- [Turnstile testing keys](https://developers.cloudflare.com/turnstile/troubleshooting/testing/)
- [Rotating a Turnstile secret key](https://developers.cloudflare.com/turnstile/troubleshooting/rotate-secret-key/)
