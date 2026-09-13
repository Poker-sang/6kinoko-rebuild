// Run through original_vm_oracle, with the original Actor and Reset methods.
Actor.funcUpdate <- null;
initCount <- 0;
ticks <- 0;
function InitProbe(value) {
    ::initCount++;
    user = { generation = ::initCount };
    SetUpdateFunction(function() { ::ticks++; });
    ::currentActor <- this;
}
actor <- CreateProbe(InitProbe);
function ResetInsideScript() {
    local previous = user;
    local generation = user.generation;
    Reset();
    if (user != previous || user.generation != generation)
        throw "original Reset changed live instance fields";
    if (::currentActor == this || ::currentActor.user == previous)
        throw "original Reset failed to create a distinct instance";
    user.finished <- true;
}
for (local index = 0; index < 32; index++) {
    ResetInsideScript.call(currentActor);
}
if (initCount != 33) throw "original Reset initialization count";
