import httplib, urllib, json

k_particle_access_token_param = 'particle_access_token'

def webhook_handler(event, context):
    
    if not k_particle_access_token_param in event:
        raise Exception("The caller must specify the '" + k_particle_access_token_param + "' parameter in the API-gateway's integration-request's mapping-templates.")
    
    particle_access_token = event['particle_access_token']
    
    request_querystring = urllib.urlencode({
        'access_token': particle_access_token,
        })
        
    request_headers = {
        "content-type": "application/json",
        "accept": "text/plain"}
    
    request_body = json.dumps({
        'name': 'web_alert',
        'data': 'github_activity',
        'private': 'false'
        })
    
    print "request_body"
    print request_body, '\n'
    
    connection = httplib.HTTPSConnection("api.particle.io")
    
    connection.request(
        "POST",
        "/v1/devices/events?%s" % request_querystring,
        request_body,
        request_headers)
        
    response = connection.getresponse()
    
    print "response_code"
    print response.status, response.reason, '\n'

    response_data = response.read()
    
    print "response_data"
    print response_data, '\n'
    
    connection.close()
