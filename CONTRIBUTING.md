# Contributing to Auto Rail Supports

Thanks for your interest. This is a solo project, so a few expectations up front.

**Open an issue first** for anything beyond a typo or an obvious bug fix. A PR that adds a feature
I wasn't planning to maintain is likely to sit unmerged, which wastes your time more than mine.

## Reporting a bug

Include:

- What you were doing (what you were building, which gun, which settings).
- What you expected vs. what happened.
- `FactoryGame.log` from `%LOCALAPPDATA%\FactoryGame\Saved\Logs\`.
- If it crashed, the folder from `%LOCALAPPDATA%\FactoryGame\Saved\Crashes\`.
- Whether it was singleplayer, hosting, or joining someone else's session. This mod behaves
  differently in each.

## Pull requests

- **One change per PR.** A PR that fixes a bug and reformats three files is hard to review.
- **Test multiplayer if you touched anything shared.** Generation is server-side and settings are
  per player. It's easy to break one while fixing the other. The SML docs have a
  multiplayer testing guide: https://docs.ficsit.app/satisfactory-modding/latest/Development/TestingResources.html

## Licence

Auto Rail Supports is GPL-3.0. By submitting a pull request you agree that your contribution is
licensed under the same terms.
