# Ternet Telegram Library

## Status

Ternet now has a **Bot API client foundation**. It validates a bot token and builds Telegram's official HTTPS API endpoint. It is not yet exposed as a Ternet `import Telegram` module and does not perform network requests yet, because the current language does not have a production module/import system or HTTP client backend.

## Target API

The intended user-facing API is:

    import Telegram

    let bot = Telegram.bot(env("BOT_TOKEN")):

    bot.on_command("start", fn(ctx) {
        ctx.reply("Hello from Ternet!"):
    }):

    bot.start():

The first real module release should cover `getMe`, `sendMessage`, message editing/deletion, callback queries, long polling, webhooks, keyboards, media, chat/member APIs, JSON response/error mapping, and configurable API base URLs.

Telegram's official Bot API is an HTTPS JSON API. The current official documentation lists Bot API 10.3 and documents the endpoint form `https://api.telegram.org/bot<TOKEN>/<METHOD>`. citeturn0search0

## Security

- Never hard-code bot tokens in `.trn` source.
- Prefer environment/secret injection.
- Never print tokens in diagnostics.
- Redact credentials from logs/errors.
- Validate Telegram's `ok`, `description`, and `error_code` response fields.
- Keep the API base URL configurable for local/test Bot API servers.

## Local Bot API server

Telegram documents a local Bot API server with larger file-transfer capabilities. The Ternet client should therefore support a configurable base URL instead of hard-coding the cloud endpoint. citeturn0search0

## Scope

This integration targets Telegram **bots** through the Bot API. A full Telegram client is a separate project using Telegram's MTProto/TDLib ecosystem; Telegram documents TDLib as a cross-platform library for custom Telegram clients. citeturn0search1
