#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

const char* alsa_device = "default";
const char* alsa_element = "Master";

//TODO: keep mixer open
int get_alsa_volume()
{
	snd_mixer_selem_id_t* sid;
	snd_mixer_elem_t* elem;
	snd_mixer_t* alsa_mixer;

	int count;

	snd_mixer_selem_id_alloca(&sid);
	
	if(snd_mixer_open(&alsa_mixer,0) < 0)
		return -1;
	if(snd_mixer_attach(alsa_mixer,alsa_device) < 0)
		return -1;
	if(snd_mixer_selem_register(alsa_mixer,NULL,NULL) < 0)
		return -1;
	if(snd_mixer_load(alsa_mixer) < 0)
		return -1;

	count = snd_mixer_get_count(alsa_mixer);
	if(count == 0)
	{
		snd_mixer_close(alsa_mixer);
		return -1;
	}
	
	elem = snd_mixer_first_elem(alsa_mixer);
	while(elem)
	{
		snd_mixer_selem_get_id(elem,sid);
		if(!strcasecmp(snd_mixer_selem_id_get_name(sid),alsa_element))
			break;
		else
			elem = snd_mixer_elem_next(elem);
	}

	if(elem)
	{
		if(!snd_mixer_selem_has_playback_volume(elem))
			return -1;

		long vol_max,vol_min;
		snd_mixer_selem_get_playback_volume_range(elem,&vol_min,&vol_max);
		
		long int vol_leftc,vol_rightc;
		snd_mixer_handle_events(alsa_mixer);
		snd_mixer_selem_get_playback_volume(elem,
				SND_MIXER_SCHN_FRONT_LEFT, &vol_leftc);
		snd_mixer_selem_get_playback_volume(elem,
				SND_MIXER_SCHN_FRONT_RIGHT, &vol_rightc);

		//TODO: both channels
		double vol_perc=0;
		vol_perc = ceil(100.0*((float)(vol_leftc-vol_min)/(float)vol_max));

		snd_mixer_close(alsa_mixer);
		return vol_perc;
	}
	else
	{
		snd_mixer_close(alsa_mixer);
		return -1;
	}
}
