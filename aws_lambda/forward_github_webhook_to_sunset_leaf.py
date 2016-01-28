import hashlib, hmac, httplib, json, urllib


def get_required_parameter(
    event, 
    parameter_name):

    if not parameter_name in event:
        raise Exception("The caller must specify the '" + parameter_name + "' parameter in the API-gateway's integration-request's mapping-templates.")
    
    return event[parameter_name]


def webhook_handler(
	event, 
	context):
    
    print '\n', "event", '\n', json.dumps(event), '\n'
    
    particle_access_token = get_required_parameter(event, 'particle_access_token')
    
    incoming_request_body = get_required_parameter(event, 'request_body')
    
    outgoing_request_querystring = urllib.urlencode({
        'access_token': particle_access_token,
        })
        
    outgoing_request_headers = {
        "content-type": "application/json",
        "accept": "text/plain",
		}
        
    outgoing_request_body = json.dumps({
        'name': 'web_alert',
        'data': 'github_activity',
        'private': 'false',
		})
    
    print '\n', "outgoing_request_body", '\n', outgoing_request_body, '\n'
    
    connection = httplib.HTTPSConnection("api.particle.io")
    
    connection.request(
        "POST",
        "/v1/devices/events?%s" % outgoing_request_querystring,
        outgoing_request_body,
        outgoing_request_headers)
        
    response = connection.getresponse()
    
    print '\n', "response_code", '\n', response.status, response.reason, '\n'

    response_data = response.read()
    
    print '\n', "response_data", '\n', response_data, '\n'
    
    connection.close()
    
    return None
