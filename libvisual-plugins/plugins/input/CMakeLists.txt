IF(ENABLE_MPLAYER)
  ADD_SUBDIRECTORY(mplayer)
ENDIF(ENABLE_MPLAYER)

IF(ENABLE_ESD)
  ADD_SUBDIRECTORY(esd)
ENDIF(ENABLE_ESD)

IF(ENABLE_JACK)
  ADD_SUBDIRECTORY(jack)
ENDIF(ENABLE_JACK)

IF(ENABLE_ALSA)
  ADD_SUBDIRECTORY(alsa)
ENDIF(ENABLE_ALSA)

IF(ENABLE_INPUT_DEBUG)
  ADD_SUBDIRECTORY(debug)
ENDIF(ENABLE_INPUT_DEBUG)

IF(ENABLE_PULSEAUDIO)
  ADD_SUBDIRECTORY(pulseaudio)
ENDIF(ENABLE_PULSEAUDIO)

IF(ENABLE_XMMS2)
  ADD_SUBDIRECTORY(xmms2)
ENDIF(ENABLE_XMMS2)
