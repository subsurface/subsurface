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

To get 'into the loop' for what is going on in Subsurface you should join our [User Forum][2], and start watching the [subsurface/subsurface repository on GitHub][1]. Conversation in the mailing list is in English – even though Subsurface itself and the website and documentation are available in many languages, the shared language the contributors communicate in is English. Actually "Broken English" is just fine… :-)


## Tips for Code Contributions

### Code Change Submissions

If you would like to contribute patches that fix bugs or add new features, that is of course especially welcome. If you are looking for places to start, look at the open bugs in our [bug tracker][5].

Here is a very brief introduction on creating commits that you can either send as [pull requests][6] on our main GitHub repository or send as emails to the mailing list. Much more details on how to use Git can be found at the [Git user manual][7].

Start with getting the latest source.

    cd subsurface
    git checkout master
    git pull

ok, now we know you're on the latest version. Create a working branch to keep your development in:

    git checkout -b devel

Edit the code (or documentation), compile, test… then create a commit:

    git commit -s -a

Depending on your OS this will open a default editor – usually you can define which by setting the environment variable `GIT_EDITOR`. Here you enter your commit message. The first line is the title of your commit. Keep it brief and to the point. Then a longer explanation (more on this and the fact that we insist on all contributions containing a Signed-off-by: line below).

If you want to change the commit message, `git commit --amend` is the way to go. Feel free to break your changes into multiple smaller commits. Then, when you are done there are two directions to go, which one you find easier depends a bit on how familiar you are with GitHub. You can either push your branch to GitHub and create a [pull requests on GitHub][6], or you run:

    git format-patch master..devel

Which creates a number of files that have names like `0001-Commit-title.patch`, which you can then send to our developer mailing list.


### Developer Certificate of Origin (DCO)

When sending code, please either send signed-off patches or a pull request with signed-off commits. If you don't sign off on them, we will not accept them. This means adding a line that says "Signed-off-by: Name \<Email\>" at the end of each commit, indicating that you wrote the code and have the right to pass it on as an open source patch.

See: [Signed-off-by Lines][8]


### Commit Messages

Also, please write good Git commit messages. A good commit message looks like this:

    Header line: explaining the commit in one line
    
    Body of commit message is a few lines of text, explaining things
    in more detail, possibly giving some background about the issue
    being fixed, etc etc.
    
    The body of the commit message can be several paragraphs, and
    please do proper word-wrap and keep columns shorter than about
    74 characters or so. That way "git log" will show things
    nicely even when it's indented.
    
    Reported-by: whoever-reported-it
    Signed-off-by: Your Name <you@example.com>

That header line really should be meaningful, and really should be just one line. The header line is what is shown by tools like gitk and shortlog, and should summarize the change in one readable line of text, independently of the longer explanation.

The preferred way to write a commit message is using [imperative mood][11], e.g. "Make foo do xyz" instead of "This patch makes foo do xyz" or "I made foo do xyz", as if you are giving commands or requests to the code base.

![gitk sample][9]

_Example with gitk_


### Coding Style

In order to make reviews simpler and have contributions merged faster in the code base, please follow Subsurface project's coding style and coding conventions described in the [CodingStyle][10] file.

[1]: https://github.com/subsurface/subsurface
[2]: https://groups.google.com/g/subsurface-divelog
[4]: https://explore.transifex.com/subsurface/subsurface/
[5]: https://github.com/subsurface/subsurface/issues
[6]: https://github.com/subsurface/subsurface/pulls
[7]: https://www.kernel.org/pub/software/scm/git/docs/user-manual.html
[8]: https://gerrit-review.googlesource.com/Documentation/user-signedoffby.html
[9]: https://github.com/subsurface/subsurface/blob/master/Documentation/images/Screenshot-gitk-subsurface-1.png "Example with gitk"
[10]: https://github.com/subsurface/subsurface/blob/master/CODINGSTYLE.md
[11]: https://en.wikipedia.org/wiki/Imperative_mood
[12]: https://app.transifex.com/subsurface/new-website/languages/
[13]: https://www.subsurface-divelog.org/latest-release/
[14]: https://github.com/subsurface/subsurface/tree/master/Documentation
[15]: https://github.com/subsurface/new-website
[16]: https://github.com/sponsors/subsurface
[17]: https://ko-fi.com/dirkhh
