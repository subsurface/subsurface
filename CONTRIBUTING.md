# Contributing to Subsurface

## Ways to Contribute

There are many ways to contribute to Subsurface. Choose the area that interests you below to find the correct workflow.

### Translate the Subsurface Application

Application translations are managed in the [Subsurface application project on Transifex][4]. Sign up for a Transifex account and request to join the language team there.

**Do not directly edit or open pull requests for `translations/subsurface_<lang>.ts`.** These files are synchronized from Transifex, so translation work submitted through GitHub cannot be incorporated into the translation workflow.

### Translate the Subsurface Website

The website has a separate [Subsurface website project on Transifex][12]. Please contribute website translations there rather than editing the website's generated translation files in a GitHub pull request.

### Test Development Versions

Testers help us find regressions and check new features on the platforms and devices that they use. We especially appreciate testing on Windows and macOS, as the majority of the active developers use Linux.

You can download the [latest development builds][13]. Builds of proposed changes are also linked from open pull requests. Report a reproducible problem in the [bug tracker][5], or use the [User Forum][2] for questions and less formal feedback.

### Improve the Documentation or Website

The source for the user manuals is in the [`Documentation` directory][14]. Corrections and improvements can be submitted as pull requests to this repository.

The project website is maintained in the separate [subsurface/new-website repository][15]. Its page content is in `src/web/templates`; follow that repository's instructions when proposing website changes. Documentation and website source changes submitted through GitHub should use signed-off commits, as described below.

### Improve the User Experience

Suggestions and contributions that improve usability, accessibility, or visual design are welcome. Start a proposal in the [bug tracker][5] or discuss it in the [User Forum][2] so that the intended behaviour can be agreed before investing in a larger implementation. Code or website changes can then be submitted to the relevant repository.

### Help Other Users

Experienced Subsurface users can contribute by joining the [User Forum][2] and helping answer questions from newer users.

### Write Code

Bug fixes and new features are submitted to this repository. See [Tips for Code Contributions](#tips-for-code-contributions) for the Git and review workflow, Developer Certificate of Origin requirements, commit-message guidance, and coding style.

### Support the Project Financially

If you would like to help cover the cost of running the free cloud synchronisation service and other project infrastructure, you can support the project through [GitHub Sponsors][16] or [Ko-fi][17].


## Joining the Subsurface Contributors' Community

New contributors can get involved by joining our [User Forum][2] and watching the [subsurface/subsurface repository on GitHub][1]. The developer mailing list is also still used occasionally by long-time contributors, but new contributors cannot subscribe to it directly.

Contributor discussions are generally in English, even though Subsurface, its website, and its documentation are available in many languages. Do not worry about writing perfect English; the community will make an effort to understand and help.


## Tips for Code Contributions

### Code Change Submissions

Bug fixes and new features are welcome. If you are looking for somewhere to start, look through the open issues in our [bug tracker][5].

The usual contribution workflow is to fork the repository on GitHub, clone your fork, and configure the main repository as the `upstream` remote. Replace `YOUR-USERNAME` with your GitHub username:

    git clone --recurse-submodules https://github.com/YOUR-USERNAME/subsurface.git
    cd subsurface
    git remote add upstream https://github.com/subsurface/subsurface.git

Before starting a change, fetch the latest upstream code and create a descriptively named topic branch from it:

    git fetch upstream
    git switch -c fix-short-description upstream/master

Make your changes, then build and test them. See [INSTALL.md](INSTALL.md) for build instructions and [README_TESTING.md](README_TESTING.md) for the available tests.

Review the changed files before staging them, then create a signed-off commit. Listing paths explicitly ensures that newly created files are included and unrelated working-tree changes are not:

    git status
    git diff
    git add path/to/changed-file path/to/new-file
    git commit -s

Git opens your configured editor for the commit message. Keep the first line brief and explain the reason for the change in the body. See [Commit Messages](#commit-messages) and the [Developer Certificate of Origin](#developer-certificate-of-origin-dco) below.

When the change is ready, push the topic branch to your fork and open a [pull request][6]:

    git push -u origin fix-short-description


### Developer Certificate of Origin (DCO)

All commits must contain a `Signed-off-by: Name <email>` line certifying that the contribution complies with the [Developer Certificate of Origin][8]. Git adds this line when you use `git commit -s`.

If you forgot the sign-off on your most recent commit, add it with `git commit --amend -s` before submitting the change.


### Commit Messages

Please write clear Git commit messages. A good commit message looks like this:

    Explain the change in one imperative sentence
    
    Body of commit message is a few lines of text, explaining things
    in more detail, possibly giving some background about the issue
    being fixed, etc etc.
    
    The body of the commit message can be several paragraphs, and
    please do proper word-wrap and keep columns shorter than about
    74 characters or so. That way "git log" will show things
    nicely even when it's indented.
    
    Reported-by: whoever-reported-it
    Signed-off-by: Your Name <you@example.com>

The first line should be meaningful on its own because it is shown in summaries and release history. Use the [imperative mood][11], such as "Fix incorrect pressure conversion" rather than "Fixed incorrect pressure conversion."

Use the body to explain why the change is needed and any decisions that are not obvious from the diff. Wrap commit-message lines at approximately 74 characters.


### Coding Style

To simplify review and help contributions get merged, follow the coding conventions in [CODINGSTYLE.md][10].

[1]: https://github.com/subsurface/subsurface
[2]: https://groups.google.com/g/subsurface-divelog
[4]: https://explore.transifex.com/subsurface/subsurface/
[5]: https://github.com/subsurface/subsurface/issues
[6]: https://github.com/subsurface/subsurface/pulls
[8]: https://developercertificate.org/
[10]: CODINGSTYLE.md
[11]: https://en.wikipedia.org/wiki/Imperative_mood
[12]: https://app.transifex.com/subsurface/new-website/languages/
[13]: https://www.subsurface-divelog.org/latest-release/
[14]: https://github.com/subsurface/subsurface/tree/master/Documentation
[15]: https://github.com/subsurface/new-website
[16]: https://github.com/sponsors/subsurface
[17]: https://ko-fi.com/dirkhh
