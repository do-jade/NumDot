extends Node2D

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	var test_size := 20000
	var test_count := 10

	var a_packed := PackedFloat64Array()
	a_packed.resize(test_size)
	var a_nd := nd.ones(test_size, nd.DType.Float64)

	### Test 1: Create packed arrays
	var start_time := Time.get_ticks_usec()
	
	# Test 3: Multiply packed arrays
	start_time = Time.get_ticks_usec()
	for t in test_count:
		for i in test_size:
			var acc := a_packed[i]
			a_packed[i] = acc * acc
	print(Time.get_ticks_usec() - start_time)
	
	# Test 4: Multiply nd arrays
	start_time = Time.get_ticks_usec()
	for t in test_count:
		nd.multiply(a_nd, a_nd)
	print(Time.get_ticks_usec() - start_time)
	
	# Test 4: Multiply nd arrays inplace
	start_time = Time.get_ticks_usec()
	for t in test_count:
		a_nd.assign_multiply(a_nd, a_nd)
	print(Time.get_ticks_usec() - start_time)
	
	# Test 5: sin packed arrays
	start_time = Time.get_ticks_usec()
	for t in test_count:
		for i in test_size:
			a_packed[i] = sin(a_packed[i])
	print(Time.get_ticks_usec() - start_time)
	
	# Test 6: sin nd arrays
	start_time = Time.get_ticks_usec()
	for t in test_count:
		nd.sin(a_nd)
	print(Time.get_ticks_usec() - start_time)

	# Test 6: sin nd arrays inplace
	start_time = Time.get_ticks_usec()
	for t in test_count:
		a_nd.assign_sin(a_nd)
	print(Time.get_ticks_usec() - start_time)


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass
