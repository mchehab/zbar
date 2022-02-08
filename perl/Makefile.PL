use 5.006;
use ExtUtils::MakeMaker;

use lib qw(inc);
use Devel::CheckLib;

check_lib_or_exit(
    lib    => 'zbar',
    header => 'zbar.h',
    LIBS   => join(' ', map({ /^LIBS=(.*)/; $1 } grep(/^LIBS=/, @ARGV))),
    INC    => join(' ', map({ /^INC=(.*)/; $1 }  grep(/^INC=/, @ARGV))),
);

WriteMakefile(
    NAME              => 'Barcode::ZBar',
    VERSION_FROM      => "ZBar.pm",
    ABSTRACT_FROM     => "ZBar.pm",
    AUTHOR            => 'Jeff Brown <spadix@users.sourceforge.net>',
    LICENSE           => 'lgpl_2_1',
    LIBS              => ['-lzbar'],
    MIN_PERL_VERSION  => '5.006',
    META_MERGE        => {
        "meta-spec" => {
            version => '2',
            url     => 'https://metacpan.org/pod/CPAN::Meta::Spec',
        },
        prereqs       => {
            build => {
                requires => {
                    'Test::More'    => 0,
                },
            },
            test => {
                recommends => {
                    'Image::Magick' => 0,
                },
            },
            develop => {
                recommends => {
                    'Test::Pod::Coverage' => 0,
                    'Test::Pod' => 0,
                },
            }
        },
        resources         => {
            homepage    => 'https://metacpan.org/pod/Barcode::ZBar/',
            repository  => {
                type => 'git',
                url  => 'https://github.com/mchehab/zbar.git',
                web  => 'https://github.com/mchehab/zbar',
            }
        },
        provides => {
            'Barcode::ZBar' => {
                file    => 'ZBar.pm',
                version => '0.10',
            },
            'Barcode::ZBar::Error' => {
                file    => 'ZBar.pm',
                version => '0.10',
            },
        }
    },
);
