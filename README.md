
This is a cleaned-up and stripped-down version of the Info-ZIP `unzip`
tool, targeting POSIX-based (aka Unix) systes.

The goal here is to be a lean, mean, unzipping machine. Non-goals include:

* Supporting non-POSIX systems. Windows support is not a goal, for example.
* Supporting obsolete systems. The original version of `unzip` claimed
support for many, many different long-dead operating systems as a point of
pride, at the expense of code readability.
* Supporting every feature that Info-ZIP supported. Some more obscure
command line arguments and features have been deleted.
* Providing anything except the `unzip` binary. The Info-ZIP version of
`unzip` provided other binaries named `funzip`, `zipcloak`, `zipgrep`,
`zipinfo`, `zipnote` and `zipsplit`. Code for all of these has been
deleted.

