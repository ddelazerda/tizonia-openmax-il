/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   icer.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - Icecast-like HTTP renderer
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>

#include <tizport.h>
#include <tizscheduler.h>
#include <tizplatform.h>

#include "icerprc.h"
#include "icermp3port.h"
#include "icercfgport.h"
#include "icer.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_renderer"
#endif

static OMX_VERSIONTYPE http_renderer_version = { {1, 0, 0, 0} };

static OMX_PTR
instantiate_mp3_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_MP3TYPE mp3type;
  OMX_AUDIO_CODINGTYPE encodings[] = {
    OMX_AUDIO_CodingMP3,
    OMX_AUDIO_CodingMax
  };
  tiz_port_options_t mp3_port_opts = {
    OMX_PortDomainAudio,
    OMX_DirInput,
    ARATELIA_HTTP_RENDERER_PORT_MIN_BUF_COUNT,
    ARATELIA_HTTP_RENDERER_PORT_MIN_BUF_SIZE,
    ARATELIA_HTTP_RENDERER_PORT_NONCONTIGUOUS,
    ARATELIA_HTTP_RENDERER_PORT_ALIGNMENT,
    ARATELIA_HTTP_RENDERER_PORT_SUPPLIERPREF,
    {ARATELIA_HTTP_RENDERER_PORT_INDEX, NULL, NULL, NULL},
    0                           /* Master port */
  };

  mp3type.nSize             = sizeof (OMX_AUDIO_PARAM_MP3TYPE);
  mp3type.nVersion.nVersion = OMX_VERSION;
  mp3type.nPortIndex        = ARATELIA_HTTP_RENDERER_PORT_INDEX;
  mp3type.nChannels         = 2;
  mp3type.nBitRate          = 128000;
  mp3type.nSampleRate       = 44100;
  mp3type.nAudioBandWidth   = 0;
  mp3type.eChannelMode      = OMX_AUDIO_ChannelModeStereo;
  mp3type.eFormat           = OMX_AUDIO_MP3StreamFormatMP1Layer3;

  return factory_new (tiz_get_type (ap_hdl, "icermp3port"),
                      &mp3_port_opts, &encodings, &mp3type);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "icercfgport"),
                      NULL,     /* this port does not take options */
                      ARATELIA_HTTP_RENDERER_COMPONENT_NAME,
                      http_renderer_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "icerprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t *rf_list[] = { &role_factory };
  tiz_type_factory_t icerprc_type;
  tiz_type_factory_t icermp3port_type;
  tiz_type_factory_t icercfgport_type;
  const tiz_type_factory_t *tf_list[] = { &icerprc_type, &icermp3port_type, &icercfgport_type};

  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX_ComponentInit: Inititializing [%s]",
           ARATELIA_HTTP_RENDERER_COMPONENT_NAME);

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_HTTP_RENDERER_DEFAULT_ROLE);
  role_factory.pf_cport   = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_mp3_port;
  role_factory.nports     = 1;
  role_factory.pf_proc    = instantiate_processor;

  strcpy ((OMX_STRING) icerprc_type.class_name, "icerprc_class");
  icerprc_type.pf_class_init = icer_prc_class_init;
  strcpy ((OMX_STRING) icerprc_type.object_name, "icerprc");
  icerprc_type.pf_object_init = icer_prc_init;

  strcpy ((OMX_STRING) icermp3port_type.class_name, "icermp3port_class");
  icermp3port_type.pf_class_init = icer_mp3port_class_init;
  strcpy ((OMX_STRING) icermp3port_type.object_name, "icermp3port");
  icermp3port_type.pf_object_init = icer_mp3port_init;

  strcpy ((OMX_STRING) icercfgport_type.class_name, "icercfgport_class");
  icercfgport_type.pf_class_init = icer_cfgport_class_init;
  strcpy ((OMX_STRING) icercfgport_type.object_name, "icercfgport");
  icercfgport_type.pf_object_init = icer_cfgport_init;

  /* Initialize the component infrastructure */
  tiz_check_omx_err (tiz_comp_init (ap_hdl, ARATELIA_HTTP_RENDERER_COMPONENT_NAME));

  /* Register the "icerprc", "icermp3port" and "icercfgport" classes */
  tiz_check_omx_err (tiz_comp_register_types (ap_hdl, tf_list, 3));

  /* Register this component's role */
  tiz_check_omx_err (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
