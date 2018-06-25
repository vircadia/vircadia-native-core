print("Loading hf-audio")

-- create the audio protocol
p_hf_audio = Proto("hf-audio", "HF Audio Protocol")

-- audio packet fields
local f_audio_sequence_number = ProtoField.uint16("hf_audio.sequence_number", "Sequence Number")
local f_audio_codec_size = ProtoField.uint32("hf_audio.codec_size", "Codec Size")
local f_audio_codec = ProtoField.string("hf_audio.codec", "Codec")
local f_audio_is_stereo = ProtoField.bool("hf_audio.is_stereo", "Is Stereo")
local f_audio_num_silent_samples = ProtoField.uint16("hf_audio.num_silent_samples", "Num Silent Samples")

p_hf_audio.fields = {
  f_audio_sequence_number, f_audio_codec_size, f_audio_codec,
  f_audio_is_stereo, f_audio_num_silent_samples
}

local packet_type_extractor = Field.new('hfudt.type_text')

function p_hf_audio.dissector(buf, pinfo, tree)
  pinfo.cols.protocol = p_hf_audio.name

  audio_subtree = tree:add(p_hf_audio, buf())

  local i = 0

  audio_subtree:add_le(f_audio_sequence_number, buf(i, 2))
  i = i + 2

  -- figure out the number of bytes the codec name takes
  local codec_name_bytes = buf(i, 4):le_uint()
  audio_subtree:add_le(f_audio_codec_size, buf(i, 4))
  i = i + 4

  audio_subtree:add(f_audio_codec, buf(i, codec_name_bytes))
  i = i + codec_name_bytes

  local packet_type = packet_type_extractor().value
  if packet_type == "SilentAudioFrame" then
    audio_subtree:add_le(f_audio_num_silent_samples, buf(i, 2))
    i = i + 2
  else
    audio_subtree:add_le(f_audio_is_stereo, buf(i, 1))
    i = i + 1
  end
end
