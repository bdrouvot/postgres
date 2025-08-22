use strict;
use warnings FATAL => 'all';

use List::Util qw(min);
use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;

#
# Create the cluster without any requested tranches
#
my $node = PostgreSQL::Test::Cluster->new('primary');
$node->init();
$node->append_conf('postgresql.conf',
    qq(shared_preload_libraries='test_tranches'));
$node->append_conf('postgresql.conf',
    qq(test_tranches.requested_named_tranches=0));
$node->start();
$node->safe_psql('postgres', q(CREATE EXTENSION test_tranches));

my $first_user_defined = $node->safe_psql('postgres', q(select test_tranches_get_first_user_defined()));
my @user_defined = ($first_user_defined .. $first_user_defined + 5);
my ($second_user_defined,
    $third_user_defined,
    $fourth_user_defined,
    $fifth_user_defined,
    $sixth_user_defined) = @user_defined[1..5];
#
# Run lookup tests to ensure the correct tranche names are returned
# and an error is raised for unregistered tranches
#
$node->safe_psql('postgres', "select test_tranches_new(5);");
my ($result, $stdout, $stderr) = $node->psql('postgres',
    qq{
        select test_tranches_lookup(1);
        select test_tranches_lookup($first_user_defined);
        select test_tranches_lookup($second_user_defined);
        select test_tranches_lookup($third_user_defined);
        select test_tranches_lookup($fourth_user_defined);
        select test_tranches_lookup($fifth_user_defined);
        select test_tranches_lookup($sixth_user_defined);
        },
    on_error_stop => 0);
like("$stderr", qr/ERROR:  tranche 100 is not registered/, "unregistered tranche error");
like("$stdout",
     qr/ShmemIndex.*test_lock__0.*test_lock__1.*test_lock__2.*test_lock__3.*test_lock__4/s,
     "match tranche names without requested tranches");

#
# Test the error for long tranche names
#
my $good_tranche_name = 'A' x 63;
my $bad_tranche_name = 'B' x 64; # MAX_NAMED_TRANCHES_NAME_LEN
$node->safe_psql('postgres', qq{select test_tranches_new_tranche('$good_tranche_name');});
($result, $stdout, $stderr) = $node->psql('postgres',
    qq{
        select test_tranches_new_tranche('$bad_tranche_name');
        },
    on_error_stop => 0);
like("$stderr", qr/ERROR:  tranche name too long/, "tranche name too long");

$node->restart();

#
# Test the error when > MAX_NAMED_TRANCHES named tranches registered.
#
$node->safe_psql('postgres', qq{select test_tranches_new(255)});
$node->safe_psql('postgres', qq{select test_tranches_new(1)});
($result, $stdout, $stderr) = $node->psql('postgres', qq{select test_tranches_new(1);}, on_error_stop => 0);
like("$stderr", qr/ERROR:  maximum number of tranches already registered/, "too many tranches registered");

#
# Repeat the lookup test with 2 requested tranches
#
$node->append_conf('postgresql.conf',
    qq(test_tranches.requested_named_tranches=2));
$node->restart();

$first_user_defined = $first_user_defined;
@user_defined = ($first_user_defined .. $first_user_defined + 5);
($second_user_defined,
    $third_user_defined,
    $fourth_user_defined,
    $fifth_user_defined,
    $sixth_user_defined) = @user_defined[1..5];

$node->safe_psql('postgres', "select test_tranches_new(3);");
($result, $stdout, $stderr) = $node->psql('postgres',
    qq{
        select test_tranches_lookup(1);
        select test_tranches_lookup($first_user_defined);
        select test_tranches_lookup($second_user_defined);
        select test_tranches_lookup($third_user_defined);
        select test_tranches_lookup($fourth_user_defined);
        select test_tranches_lookup($fifth_user_defined);
        select test_tranches_lookup($sixth_user_defined);
        },
    on_error_stop => 0);
like("$stderr", qr/ERROR:  tranche 100 is not registered/, "unregistered tranche error");
like("$stdout", qr/ShmemIndex.*test_lock_0.*test_lock_1.*test_lock__2.*test_lock__3.*test_lock__4/s,
     "match tranche names with requested tranches");

#
# Test lwlock initialize
#
$node->safe_psql('postgres', qq{select test_tranches_lwlock_initialize($first_user_defined)});
($result, $stdout, $stderr) = $node->psql('postgres',
    qq{select test_tranches_lwlock_initialize($sixth_user_defined)},
    on_error_stop => 0);
like("$stderr", qr/ERROR:  tranche 100 is not registered/, "LWLock intialization error on invalid tranche name");

#
# Test error for NULL tranche name
#
($result, $stdout, $stderr) = $node->psql('postgres',
    qq{select test_tranches_new_tranche(NULL)},
    on_error_stop => 0);
like("$stderr", qr/ERROR:  tranche name cannot be NULL/, "NULL tranche name");

done_testing();
