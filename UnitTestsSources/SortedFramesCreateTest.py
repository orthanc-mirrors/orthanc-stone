#!/usr/bin/env python

import pprint
import requests
import sys

if len(sys.argv) != 2:
    print('Usage: %s [Orthanc series ID]' % sys.argv[0])
    print('Example: %s 4d04593b-953ced51-87e93f11-ae4cf03c-25defdcd | xclip -selection c -t text/plain' % sys.argv[0])
    exit(-1)

SERIES = sys.argv[1]

r = requests.get('http://localhost:8042/series/%s/study' % SERIES)
r.raise_for_status()
print('  // From patient %s' % r.json() ['PatientMainDicomTags']['PatientName'])

r = requests.get('http://localhost:8042/series/%s' % SERIES)
r.raise_for_status()

first = True

for instance in r.json() ['Instances']:
    tags = requests.get('http://localhost:8042/instances/%s/tags?short' % instance)
    tags.raise_for_status()

    if first:
        print('''
  Orthanc::DicomMap tags;
  tags.SetValue(Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, "%s", false);
  tags.SetValue(Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, "%s", false);    
  OrthancStone::SortedFrames f;
''' % (tags.json() ['0020,000d'], tags.json() ['0020,000e']))
        first = False
   
    print('  tags.SetValue(Orthanc::DICOM_TAG_SOP_INSTANCE_UID, "%s", false);' % instance)
    
    if '0020,0032' in tags.json():
        print('  tags.SetValue(Orthanc::DICOM_TAG_IMAGE_POSITION_PATIENT, "%s", false);' %
              tags.json() ['0020,0032'].replace('\\', '\\\\'))
    
    if '0020,0037' in tags.json():
        print('  tags.SetValue(Orthanc::DICOM_TAG_IMAGE_ORIENTATION_PATIENT, "%s", false);' %
              tags.json() ['0020,0037'].replace('\\', '\\\\'))

    if '0020,0013' in tags.json():
        print('  tags.SetValue(Orthanc::DICOM_TAG_INSTANCE_NUMBER, "%s", false);' %
              tags.json() ['0020,0013'].replace('\\', '\\\\'))

    if '0054,1330' in tags.json():
        print('  tags.SetValue(Orthanc::DICOM_TAG_IMAGE_INDEX, "%s", false);' %
              tags.json() ['0054,1330'].replace('\\', '\\\\'))

    print('  f.AddInstance(tags);')
    
print('  f.Sort();')

r = requests.get('http://localhost:8042/series/%s/ordered-slices' % SERIES)
r.raise_for_status()
slices = r.json() ['SlicesShort']

print('  ASSERT_EQ(%du, f.GetFramesCount());' % len(slices))

for i in range(len(slices)):
    print('  ASSERT_EQ(f.GetFrameSopInstanceUid(%d), "%s");' % (i, slices[i][0]))

