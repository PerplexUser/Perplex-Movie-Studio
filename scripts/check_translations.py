#!/usr/bin/env python3
import pathlib, re, xml.etree.ElementTree as ET
path=pathlib.Path(__file__).resolve().parents[1]/'translations/perplex_de_DE.ts'
tree=ET.parse(path);count=0
for context in tree.getroot().findall('context'):
    for message in context.findall('message'):
        source=message.findtext('source') or '';translation=message.find('translation')
        assert translation is not None and translation.text and translation.get('type') not in ('unfinished','vanished','obsolete'),f'Missing translation: {source}'
        assert sorted(re.findall(r'%\d+',source))==sorted(re.findall(r'%\d+',translation.text)),f'Placeholder mismatch: {source}'
        count+=1
print(f'PASS {count} finished translations with matching placeholders')
