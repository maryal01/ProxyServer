# Image filtering for images based on url

import io
import os
import sys
import getopt

opts, args = getopt.getopt(['-a', '-m', '-r', '-s', '-v'], 'amrsv')

likelihood_name = ('UNKNOWN', 'VERY_UNLIKELY', 'UNLIKELY', 'POSSIBLE',
                       'LIKELY', 'VERY_LIKELY')

# given options to filter adult, medical, racy, spoof, and violence
# returns true if it does
def detect_web_uri():
    """Detects web annotations in the file located in Google Cloud Storage."""
    from google.cloud import vision
    from google.cloud.vision import types
    client = vision.ImageAnnotatorClient()
    image = vision.types.Image()
    image.source.image_uri = sys.argv[len(sys.argv) - 1]

    response = client.safe_search_detection(image=image)
    annotations = response.safe_search_annotation

    adult = likelihood_name[annotations.adult]
    medical = likelihood_name[annotations.medical]
    spoof = likelihood_name[annotations.spoof]
    violence = likelihood_name[annotations.violence]
    racy = likelihood_name[annotations.racy]

    print(adult)
    print(medical)
    print(racy)
    print(spoof)
    print(violence)

    if '-a' in sys.argv:
        if 'UNLIKELY' not in adult:
            print(adult)
            return False

    if '-m' in sys.argv:
        if 'UNLIKELY' not in medical:
            print(medical)
            return False

    if '-r' in sys.argv:
        if 'UNLIKELY' not in racy:
            print(racy)
            return False

    if '-s' in sys.argv:
        if 'UNLIKELY' not in spoof:
            print(spoof)
            return False

    if '-v' in sys.argv:
        if 'UNLIKELY' not in violence:
            print(violence)
            return False
    
    return True

if detect_web_uri():
    print('clean')
else:
    print('unclean')
