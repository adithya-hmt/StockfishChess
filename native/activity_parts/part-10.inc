
__attribute__((visibility("default")))
void ANativeActivity_onCreate(ANativeActivity*activity,void*savedState,size_t savedStateSize){
    App*a=&g_app;(void)savedState;(void)savedStateSize;memset(a,0,sizeof(*a));a->timer_pipe[0]=a->timer_pipe[1]=-1;a->notify_pipe[0]=a->notify_pipe[1]=-1;a->activity=activity;a->window=0;a->selected=-1;a->hint_from=a->hint_to=-1;a->last_from=a->last_to=-1;a->history_index=-1;a->profile_avatar=0;a->profile_accent=0;a->onboarding_page=0;cer_engine_init(&a->engine);
    cer_copy_text(a->data_dir,sizeof(a->data_dir),activity->internalDataPath?activity->internalDataPath:"/data/local/tmp");cer_copy_text(a->export_dir,sizeof(a->export_dir),activity->externalDataPath?activity->externalDataPath:a->data_dir);cer_state_load(a->data_dir,&a->saved);
    a->setup_mode=MODE_ENGINE;a->setup_side=a->saved.default_side;a->setup_level=a->saved.default_level;a->setup_time=a->saved.default_time;restore_active_game(a);a->screen=SCR_SPLASH;a->tab=TAB_HOME;
    activity->instance=a;activity->callbacks->onStart=on_start;activity->callbacks->onResume=on_resume;activity->callbacks->onPause=on_pause;activity->callbacks->onStop=on_stop;activity->callbacks->onDestroy=on_destroy;activity->callbacks->onNativeWindowCreated=on_window_created;activity->callbacks->onNativeWindowResized=on_window_resized;activity->callbacks->onNativeWindowRedrawNeeded=on_window_redraw;activity->callbacks->onNativeWindowDestroyed=on_window_destroyed;activity->callbacks->onInputQueueCreated=on_input_created;activity->callbacks->onInputQueueDestroyed=on_input_destroyed;
    ANativeActivity_setWindowFlags(activity,0x00000480u,0);ANativeActivity_setWindowFormat(activity,WINDOW_FORMAT_RGBA_8888);timer_start(a);worker_init(a);
}
