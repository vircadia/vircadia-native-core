def hifiJob(String targetName, Boolean deploy) {
    def JENKINS_URL = 'https://jenkins.below92.com/'
    def GITHUB_HOOK_URL = 'https://github.com/worklist/hifi/'
    def GIT_REPO_URL = 'git@github.com:worklist/hifi.git'
    def HIPCHAT_ROOM = 'High Fidelity'
    
    job {
        name "hifi-${targetName}"
        logRotator(7, -1, -1, -1)
        
        scm {
            git(GIT_REPO_URL, 'master') { node ->
                 node / includedRegions << "${targetName}/.*\nlibraries/.*"
                 node / 'userRemoteConfigs' / 'hudson.plugins.git.UserRemoteConfig' / 'name' << ''
                 node / 'userRemoteConfigs' / 'hudson.plugins.git.UserRemoteConfig' / 'refspec' << ''
            }
        }
       
        configure { project ->
            project / 'properties' << {
                'com.coravy.hudson.plugins.github.GithubProjectProperty' {
                    projectUrl GITHUB_HOOK_URL
                }
                
                'jenkins.plugins.hipchat.HipChatNotifier_-HipChatJobProperty' {
                    room HIPCHAT_ROOM
                } 
                
                'hudson.plugins.buildblocker.BuildBlockerProperty' {
                    useBuildBlocker true
                    blockingJobs 'hifi--seed'
                }         
            }
            
            project / 'triggers' << 'com.cloudbees.jenkins.GitHubPushTrigger' {
                spec ''
            }
            
            project / 'builders' << 'hudson.plugins.cmake.CmakeBuilder' {
                sourceDir targetName
                buildDir 'build'
                installDir ''
                buildType 'RelWithDebInfo'
                generator 'Unix Makefiles'
                makeCommand 'make'
                installCommand 'make install'
                preloadScript ''
                cmakeArgs ''
                projectCmakePath '/usr/bin/cmake'
                cleanBuild 'false'
                cleanInstallDir 'false'
                builderImpl ''
            }
        }
        
        if (deploy) {
            publishers {            
                publishScp("${ARTIFACT_DESTINATION}") {
                    entry("**/build/${targetName}", "deploy/${targetName}")
                }
            }
        }
        
        configure { project ->
            
            project / 'publishers' << {
                if (deploy) {
                    'hudson.plugins.postbuildtask.PostbuildTask' {
                        'tasks' {
                            'hudson.plugins.postbuildtask.TaskProperties' {
                                logTexts {
                                    'hudson.plugins.postbuildtask.LogProperties' {
                                        logText '.'
                                        operator 'AND'
                                    }
                                }
                                EscalateStatus true
                                RunIfJobSuccessful true
                                script "curl -d 'action=deploy&role=highfidelity-live&revision=${targetName}' https://${ARTIFACT_DESTINATION}"
                            }
                        }
                    }
                }
                
                'jenkins.plugins.hipchat.HipChatNotifier' {
                    jenkinsUrl JENKINS_URL
                    authToken "${HIPCHAT_AUTH_TOKEN}"
                    room HIPCHAT_ROOM
                }
            }
        }
    }
}

def targets = [
    'animation-server':true,
    'audio-mixer':true,
    'avatar-mixer':true,
    'domain-server':true,
    'eve':true,
    'pairing-server':true,
    'space-server':true,
    'voxel-server':true,
    'interface':false,
    'dummy-job':false
]

/* setup all of the target jobs to use the above template */
for (target in targets) {
    queue hifiJob(target.key, target.value)
}

/* setup the parametrized-build job for builds from jenkins */
parameterizedJob = hifiJob('$TARGET', true)
parameterizedJob.with {
    name 'hifi-branch-deploy'
    parameters {
        stringParam('GITHUB_USER', '', "Specifies the name of the GitHub user that we're building from.")
        stringParam('GIT_BRANCH', '', "Specifies the specific branch to build and deploy.")
        stringParam('HOSTNAME', 'devel.highfidelity.io', "Specifies the hostname to deploy against.")
        stringParam('TARGET', '', "What server to build specifically")
    }   
    scm {
        git('git@github.com:/$GITHUB_USER/hifi.git', '$GIT_BRANCH') { node ->
            node / 'wipeOutWorkspace' << true
        }
    } 
}