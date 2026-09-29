Downloading ZBar source code
============================

When hacking at ZBar, PLEASE send patches against the latest Git tree!

There are currently 3 mirrors with ZBar:

LinuxTV:
    Cgit GUI interface:
	https://git.linuxtv.org/zbar.git/
    Git tree:
	git://linuxtv.org/zbar.git
	https://git.linuxtv.org/cgit.cgi/zbar.git
	http://git.linuxtv.org/cgit.cgi/zbar.git

Github:
	https://github.com/mchehab/zbar.git

Gitlab:
	https://github.com/mchehab/zbar

You can use git clone to get the latest version from any of the
above repositories.

If you haven't already, grab the ZBar git repository. For example:

    git clone https://github.com/mchehab/zbar.git
    cd zbar
    meson setup builddir
    meson compile -C builddir

Meson and Ninja are required. Optional feature dependencies are detected by
Meson and can be selected or disabled with `meson configure builddir`.

Writing descriptions for your patches
=====================================

Please add a good description to your patch adding why it is needed,
what the patch does and how. This helps us when reviewing your work
when you submit upstream.

We use a process similar to the Linux Kernel for patch submissions.
In particular, submitted patches should have a developer's certificate
of origin, as described at:
	https://linuxtv.org/wiki/index.php/Development:_Submitting_Patches#Developer.27s_Certificate_of_Origin_1.1

In practice, please add:

	Signed-off-by: your name <your@email>

on your patches.

Submitting patches
==================

When you're done hacking, please submit your work back upstream.

If you use Github or Gitlab, you can fork ZBar from it, develop your
patches and then push again to your clone, asking the patch merge using
the GUI.

Although we prefer if you submit patches via either Github or
Gitlab, you can also submit them via e-mail to:

	linux-media@vger.kernel.org

If you opt to do so, please place [PATCH ZBar] at the subject of
your e-mails for us to not mix them with patches for the Kernel
or for other media tools.

To make your patch, run:

    git diff > hacked.patch

Other things for you to read, in order to know more about how
to submit your work for upstreaming processes in general, that
could be useful for you to prepare yourself on submitting patches
to ZBar:

- https://linuxtv.org/wiki/index.php/Development:_Submitting_Patches
- http://www.faqs.org/docs/artu/ch19s02.html
- http://www.catb.org/~esr/faqs/smart-questions.html
