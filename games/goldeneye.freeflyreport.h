/* Called by the input thread. No persistent settings or game-memory writes. */
void GE_DebugInput(FILE *out)
{
    const GE_ADDRESS_PROFILE *profile;
    const GE_MAPMAKER_PROFILE *editor;
    if(!romptr || !rdramptr) {
        fprintf(out, "ROM/RAM hook not ready: ROM=%d RAM=%d\n", !!romptr, !!rdramptr);
        return;
    }
    profile = GE_GetAddressProfile();
    editor = &profile->mapmaker;
    fprintf(out, "GEProfile=%d MaxPage=%u MapMakerPage=%u MenuRows=%u\n",
        !!profile->bonddata, profile->maxpage, editor->page, editor->menu_rows);
    if(profile->bonddata)
        fprintf(out, "CurrentPage=%d Camera=%d Pause=%d Exit=%d MenuCursor=%.6g,%.6g\n",
            EMU_ReadInt(profile->menupage), EMU_ReadInt(profile->camera),
            EMU_ReadInt(profile->pause), EMU_ReadInt(profile->exit),
            EMU_ReadFloat(profile->menux), EMU_ReadFloat(profile->menuy));
    if(editor->page) {
        fprintf(out, "FreeFly=%d EditorMenu=%d Preview=%d NextPage=%d AlternateNextPage=%d\n",
            EMU_ReadInt(editor->freemode), EMU_ReadInt(editor->menu),
            EMU_ReadInt(editor->preview), EMU_ReadInt(editor->nextpage), EMU_ReadInt(editor->nextpagealt));
        fprintf(out, "FreeFlyAngles=%.9g,%.9g PitchLimits=%.9g,%.9g\n",
            EMU_ReadFloat(editor->yaw), EMU_ReadFloat(editor->pitch),
            EMU_ReadFloat(editor->pitchmin), EMU_ReadFloat(editor->pitchmax));
    }
    fprintf(out, "LookVisits=%lu YawWrites=%lu PitchWrites=%lu\nLastLookResult=%s\n"
        "LastYawChange=%.9g -> %.9g LastPitchChange=%.9g -> %.9g\n",
        ge_ff_visits, ge_ff_yaw_writes, ge_ff_pitch_writes, ge_ff_result,
        ge_ff_before_yaw, ge_ff_after_yaw, ge_ff_before_pitch, ge_ff_after_pitch);
}
