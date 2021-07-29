function escape_xml_for_html(val)
{
  return val.replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/\ /g, '&nbsp;')
            .replace(/\r\n/g, '<br>');
}

function escape_text_for_xml(val)
{
  return val.replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;');
}

function hash_djb2(val)
{
  // djb2 using xor; http://www.cse.yorku.ca/~oz/hash.html
  var hash = 5381;
  for (i = 0; i < val.length; i++)
    hash = ((hash << 5) + hash) ^ val.charCodeAt(i);

  return ("00000000" + (hash >>> 0).toString(16)).slice(-8);
}

function clean_description(val)
{
  var clean_desc = "";
  for (i = 0; i < val.length && i < 30; i++) {
    if (val.charCodeAt(i) >= 0x20 && val.charCodeAt(i) <= 0x7e)
      clean_desc += val.charAt(i);
  }
  return clean_desc;
}


function create_rdd6_xml(description, dialnorm, dyn_range_pf, compr_pf,
                         center_mix_level, sur_mix_level, sur_att_filter,
                         sur_encoded, downmix_mode, sur_90_filter)
{
  return '<?xml version="1.0" encoding="utf-8"?>\r\n' +
   '<rdd6 xmlns="http://bbc.co.uk/rd/rdd6/201502">\r\n' +
   '  <first_subframe>\r\n' +
   '   <sync>\r\n' +
   '     <rev_id>0x00</rev_id>\r\n' +
   '     <orig_id>0x01</orig_id>\r\n' +
   '     <orig_addr>0x0000</orig_addr>\r\n' +
   '     <frame_count>201</frame_count>\r\n' +
   '   </sync>\r\n' +
   '   <dolby_e_complete>\r\n' +
   '     <program_config>5.1</program_config>\r\n' +
   '     <frame_rate>25</frame_rate>\r\n' +
   '     <descr_text>\r\n' +
   '       <program>' + escape_text_for_xml(clean_description(description)) + '</program>\r\n' +
   '     </descr_text>\r\n' +
   '   </dolby_e_complete>\r\n' +
   '   <dolby_digital_complete_ext_bsi>\r\n' +
   '     <program_id>0</program_id>\r\n' +
   '     <ac_mode>3/2</ac_mode>\r\n' +
   '     <bs_mode>main_complete</bs_mode>\r\n' +
   '     <center_mix_level>' + center_mix_level + '</center_mix_level>\r\n' +
   '     <sur_mix_level>' + sur_mix_level + '</sur_mix_level>\r\n' +
   '     <sur_encoded>' + sur_encoded + '</sur_encoded>\r\n' +
   '     <lfe_on>true</lfe_on>\r\n' +
   '     <dialnorm>' + dialnorm + '</dialnorm>\r\n' +
   '     <copyright>true</copyright>\r\n' +
   '     <orig_bs>true</orig_bs>\r\n' +
   '     <downmix_mode>' + downmix_mode + '</downmix_mode>\r\n' +
   '     <lt_rt_center_mix>' + center_mix_level + '</lt_rt_center_mix>\r\n' +
   '     <lt_rt_sur_mix>' + sur_mix_level + '</lt_rt_sur_mix>\r\n' +
   '     <lo_ro_center_mix>' + center_mix_level + '</lo_ro_center_mix>\r\n' +
   '     <lo_ro_sur_mix>' + sur_mix_level + '</lo_ro_sur_mix>\r\n' +
   '     <ad_conv_type>standard</ad_conv_type>\r\n' +
   '     <hp_filter>true</hp_filter>\r\n' +
   '     <bw_lp_filter>true</bw_lp_filter>\r\n' +
   '     <lfe_lp_filter>true</lfe_lp_filter>\r\n' +
   '     <sur_90_filter>' + sur_90_filter + '</sur_90_filter>\r\n' +
   '     <sur_att_filter>' + sur_att_filter + '</sur_att_filter>\r\n' +
   '     <rf_preemph_filter>false</rf_preemph_filter>\r\n' +
   '     <compr_pf_1>' + compr_pf + '</compr_pf_1>\r\n' +
   '     <dyn_range_pf_1>' + dyn_range_pf + '</dyn_range_pf_1>\r\n' +
   '     <dyn_range_pf_2>none</dyn_range_pf_2>\r\n' +
   '     <dyn_range_pf_3>none</dyn_range_pf_3>\r\n' +
   '     <dyn_range_pf_4>none</dyn_range_pf_4>\r\n' +
   '   </dolby_digital_complete_ext_bsi>\r\n' +
   '  </first_subframe>\r\n' +
   '  <second_subframe>\r\n' +
   '   <sync>\r\n' +
   '     <rev_id>0x00</rev_id>\r\n' +
   '     <orig_id>0x01</orig_id>\r\n' +
   '     <orig_addr>0x0000</orig_addr>\r\n' +
   '     <frame_count>201</frame_count>\r\n' +
   '   </sync>\r\n' +
   '   <dolby_e_essential>\r\n' +
   '     <program_config>5.1</program_config>\r\n' +
   '     <frame_rate>25</frame_rate>\r\n' +
   '   </dolby_e_essential>\r\n' +
   '   <dolby_digital_essential_ext_bsi>\r\n' +
   '     <program_id>0</program_id>\r\n' +
   '     <ac_mode>3/2</ac_mode>\r\n' +
   '     <bs_mode>main_complete</bs_mode>\r\n' +
   '     <lfe_on>true</lfe_on>\r\n' +
   '     <dialnorm>' + dialnorm + '</dialnorm>\r\n' +
   '     <compr_pf_2>' + compr_pf + '</compr_pf_2>\r\n' +
   '     <dyn_range_pf_5>' + dyn_range_pf + '</dyn_range_pf_5>\r\n' +
   '     <dyn_range_pf_6>none</dyn_range_pf_6>\r\n' +
   '     <dyn_range_pf_7>none</dyn_range_pf_7>\r\n' +
   '     <dyn_range_pf_8>none</dyn_range_pf_8>\r\n' +
   '   </dolby_digital_essential_ext_bsi>\r\n' +
   '  </second_subframe>\r\n' +
   '</rdd6>';
}


$("#description").focusout(function() {
    $("#description").val(clean_description($("#description").val()));
});

$("#audiometadata").submit(function(event) {
    var rdd6_xml = create_rdd6_xml($("#description").val(),
                                   $("#dialogueLevel").val(),
                                   $("#lineModeCompression").val(),
                                   $("#rfModeCompression").val(),
                                   $("#centreDownMixLevel").val(),
                                   $("#surroundDownMixLevel").val(),
                                   $("#surround3DBAttn").val(),
                                   $("#dolbySurroundMode").val(),
                                   $("#prefStereoDownMix").val(),
                                   $("#surroundPhaseShift").val());

    var filename = "dpp_rdd6_" + hash_djb2(rdd6_xml) + ".xml";
    if (saveAs && Blob) {
      var blob = new Blob([rdd6_xml], {type: "application/xml;charset=utf-8"});
      saveAs(blob, filename);
    } else {
      // <= IE9
      saveTextAs(escape_xml_for_html(rdd6_xml), filename, "utf-8");
    }

    event.preventDefault();
});
