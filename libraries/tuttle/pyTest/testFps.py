from pyTuttle.tuttle import *

def setUp():
	core().preload()


def testFpsPropagation():

	return # TODO

	compute( [
		NodeInit( "tuttle.ffmpegreader", filename="TuttleOFX-data/video/morgen-20030714.avi" ),
		NodeInit( "tuttle.invert" ),
		NodeInit( "tuttle.invert" ),
		NodeInit( "tuttle.ffmpegwriter", filename=".tests/output.avi" ),
		] )
	
	# TODO: check the fps! ;)
