# create the target
add_pdb_integration_test(TestCartasianJoinWithKey)

# add a build dependency to build-tests target
add_dependencies(build-integration-tests TestCartasianJoinWithKey)

# add build dependencies to shared libraries it uses
add_dependencies(TestCartasianJoinWithKey StringStringPair)
add_dependencies(TestCartasianJoinWithKey ReadStringStringPair)
add_dependencies(TestCartasianJoinWithKey SillyCartasianJoinStringString)
add_dependencies(TestCartasianJoinWithKey SillyWriteStringString)
