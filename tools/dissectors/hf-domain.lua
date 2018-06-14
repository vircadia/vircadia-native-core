-- create the domain protocol
p_hf_domain = Proto("hf-domain", "HF Domain Protocol")

-- domain packet fields
local f_domain_id = ProtoField.guid("hf_domain.domain_id", "Domain ID")
local f_domain_local_id = ProtoField.uint16("hf_domain.domain_local_id", "Domain Local ID")

p_hf_domain.fields = {
  f_domain_id, f_domain_local_id
}

function p_hf_domain.dissector(buf, pinfo, tree)
  pinfo.cols.protocol = p_hf_domain.name

  domain_subtree = tree:add(p_hf_domain, buf())

  local i = 0

  domain_subtree:add(f_domain_id, buf(i, 16))
  i = i + 16

  domain_subtree:add_le(f_domain_local_id, buf(i, 2))
end
