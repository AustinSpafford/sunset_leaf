Finding your Particle Access Token:

	As seen here:
	https://docs.particle.io/reference/api/#authentication
	...refer to "Your access token can be found in the Particle Build web IDE on the 'Settings' page."
	
	NOTE: It should be possible to generate additional (and controllable) access tokens instead of using the core access token.


Setting up the Stage Variables:

	As seen here:
	http://docs.aws.amazon.com/apigateway/latest/developerguide/how-to-set-stage-variables-aws-console.html
	...refer to step 4, adding:
	
	"particle_access_token", "<the_access_token_value_from_a_previous_step>"
	

Passing the Stage Variables through to the Lambda Function:

	As seen here:
	http://docs.aws.amazon.com/apigateway/latest/developerguide/amazon-api-gateway-using-stage-variables.html
	...in the "Pass stage-specific metadata to a Lambda function via a stage variable" section, we set the mapping template to:

	{
		"particle_access_token": "$stageVariables.particle_access_token",
		"query_string": "$input.params().querystring"
	}