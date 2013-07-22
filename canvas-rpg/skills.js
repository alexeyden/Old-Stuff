game.skills.push(new skill("Kick","Kick object",2,
function()
{
    game.ui.log_add("kick ass");
}));
game.skills.push(new skill("Heal","Restores 5*Level HP",5,
function()
{
    game.ui.log_add("now healing...");
}));
