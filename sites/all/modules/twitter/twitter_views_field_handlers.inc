<?php
/**
 * @file
 * Views handlers for Twitter module.
 */

/**
 * Process Twitter-style @usernames and URLs before filtering XSS.
 */
class twitter_views_handler_field_xss extends views_handler_field {
  function option_definition() {
    $options = parent::option_definition();
    $options['link_urls'] = array('default' => TRUE);
    $options['link_usernames'] = array('default' => TRUE);
    $options['link_hashtags'] = array('default' => TRUE);
    $options['link_attributes'] = array('default' => TRUE);
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['link_urls'] = array(
      '#title' => t('Link urls to their destinations'),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['link_urls']),
    );
    $form['link_usernames'] = array(
      '#title' => t('Link Twitter @usernames to their Twitter.com urls'),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['link_usernames']),
    );
    $form['link_hashtags'] = array(
      '#title' => t('Link Twitter #hashtags to another url'),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['link_hashtags']),
    );
    $form['link_attributes'] = array(
      '#title' => t('Open links in new windows/tabs and add rel="nofollow" attributes.'),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['link_attributes']),
    );
  }

  /**
   * Processes the message through the selected options.
   */
  function render($values) {
    $value = $values->{$this->field_alias};

    // Load the options.
    $usernames = TRUE;
    if (isset($this->options['link_usernames'])) {
      $usernames = $this->options['link_usernames'];
    }
    $hashtags = TRUE;
    if (isset($this->options['link_hashtags'])) {
      $hashtags = $this->options['link_hashtags'];
    }
    $attributes = TRUE;
    if (isset($this->options['link_attributes'])) {
      $attributes = $this->options['link_attributes'];
    }
    $urls = TRUE;
    if (isset($this->options['link_urls'])) {
      $urls = $this->options['link_urls'];
    }

    // Offload processing to the shared function.
    return twitter_filter_message($value, $usernames, $hashtags, $attributes, $urls);
  }
}

/**
 * Field handler to provide simple renderer that turns a URL into a clickable link.
 */
class twitter_views_handler_field_profile_image extends views_handler_field {
  function render($values) {
    $value = $values->{$this->field_alias};
    $output = theme('image', array('path' => $value, 'width' => 48, 'height' => 48));

    // Convert to a protocol-relative URL so that the same image tag will work
    // regardless of whether the page is loaded via HTTP or HTTPS.
    return str_replace('http:', '', $output);
  }
}

/**
 * Adds Twitter Intents links.
 *
 * @see https://dev.twitter.com/docs/intents
 */
class twitter_views_handler_field_web_intents extends views_handler_field {
  /**
   * Add twitter_id field, which is needed during rendering.
   */
  function construct() {
    parent::construct();
    $this->additional_fields['twitter_id'] = 'twitter_id';
  }

  function query() {
    $this->ensure_my_table();
    $this->add_additional_fields();
  }

  function render($values) {
    drupal_add_js('//platform.twitter.com/widgets.js', 'external');
    return '<span><a href="https://twitter.com/intent/tweet?in_reply_to=' . $values->twitter_id . '">' . t('Reply') . '</a></span> ' .
      '<span><a href="https://twitter.com/intent/retweet?tweet_id=' . $values->twitter_id . '">' . t('Retweet') . '</a></span> ' .
      '<span><a href="https://twitter.com/intent/favorite?tweet_id=' . $values->twitter_id . '">' . t('Favorite') . '</a></span>';
  }
}

/**
 * Adds Twitter Follow link.
 *
 * @see https://dev.twitter.com/docs/intents#follow-intent
 */
class twitter_views_handler_field_follow extends views_handler_field {

  function query() {}

  function render($values) {
    // Try to find the screen name.
    $screen_name = '';

    // If the screen name is available from the view then our work is done.
    if (!empty($values->twitter_screen_name)) {
      $screen_name = $values->twitter_screen_name;
    }

    // If the twitter_id is passed in, look for this tweet and get the screen
    // name associated with it.
    elseif (!empty($values->twitter_id)) {
      $screen_name = db_query("SELECT screen_name FROM {twitter} WHERE twitter_id = :id", array(':id' => $values->twitter_id))->fetchField();
    }

    // If the created_time is passed in, look for this tweet and get the screen
    // name associated with it. This may result in multiple values,
    // unfortunately there aren't too many ways around this so just pick the
    // first one.
    elseif (!empty($values->twitter_created_time)) {
      $screen_name = db_query("SELECT screen_name FROM {twitter} WHERE created_time = :time", array(':time' => $values->twitter_created_time))->fetchField();
    }

    // Only output something if it was possible to identify the screen name.
    if (!empty($screen_name)) {
      drupal_add_js('//platform.twitter.com/widgets.js', 'external');
      return '<span><a href="https://twitter.com/intent/user?screen_name=' . $screen_name . '">' . t('Follow') . '</a></span>';
    }
    else {
      // Unable to identify the screen name for this tweet.
    }
  }
}

/**
 * Renders a tweet as it is presented at Twitter.com.
 *
 * @see https://dev.twitter.com/terms/display-requirements
 */
class twitter_views_handler_field_formatted_tweet extends views_handler_field {
  function query() {}

  function render($values) {
    drupal_add_js('//platform.twitter.com/widgets.js', 'external');
    drupal_add_css(drupal_get_path('module', 'twitter') . '/twitter.css');
    module_load_include('inc', 'twitter');

    // Load tweet and author.
    $twitter_status = entity_load_single('twitter_status', $values->twitter_id);
    $twitter_account = entity_load('twitter_account', FALSE, array('screen_name' => $twitter_status->screen_name));
    $twitter_account = reset($twitter_account);

    // Render the tweet.
    return theme('twitter_status', array(
      'status' => $twitter_status,
      'author' => $twitter_account,
      'reply' => t('Reply'),
      'retweet' => t('Retweet'),
      'favorite' => t('Favorite'),
    ));
  }
}
